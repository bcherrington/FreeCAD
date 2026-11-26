// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <fstream>
#include <stdexcept>

#include "App/Application.h"
#include "App/Document.h"
#include "App/DocumentObject.h"
#include "App/Datums.h"
#include "App/Origin.h"
#include "App/Part.h"
#include "App/Expression.h"
#include "App/MergeDocuments.h"
#include "App/PropertyUnits.h"
#include "App/StringHasher.h"
#include "Base/Tools.h"
#include "Base/Writer.h"
#include "App/private/RecomputeAdmission.h"
#include <src/App/InitApplication.h>

using ::testing::Eq;
using ::testing::Ne;

// NOLINTBEGIN(readability-magic-numbers)

class FakeWriter: public Base::Writer
{
    void writeFiles() override
    {}
    std::ostream& Stream() override
    {
        return std::cout;
    }
};

class DocumentTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    App::Document* doc()
    {
        return _doc;
    }

private:
    std::string _docName;
    App::Document* _doc {};
};


TEST_F(DocumentTest, RestoreProtectsReceiverThroughCallbacks)
{
    auto& app = App::GetApplication();
    const std::string name = doc()->getName();
    const std::string path = App::Application::getTempFileName();
    ASSERT_TRUE(doc()->saveAs(path.c_str()));
    const std::string savedPath = doc()->getFileName();
    Base::ScopeGuard removeFile([&] { std::remove(savedPath.c_str()); });
    unsigned callbacks = 0;
    auto checkLifetime = [&] {
        auto& coordinator = App::RecomputeDetail::Coordinator::instance();
        // Do not destroy the receiver when demonstrating the pre-fix failure.
        const bool protectedNow = coordinator.isObserved(coordinator.bindDocument(doc()));
        EXPECT_TRUE(protectedNow);
        if (protectedNow) {
            EXPECT_FALSE(app.closeDocument(name.c_str()));
            EXPECT_EQ(app.getDocument(name.c_str()), doc());
        }
        ++callbacks;
    };
    auto start = app.signalStartRestoreDocument.connect([&](const App::Document& document) {
        if (&document == doc()) {
            checkLifetime();
        }
    });
    auto files = doc()->signalRestoreDocument.connect([&](Base::XMLReader&) { checkLifetime(); });
    auto finish = app.signalFinishRestoreDocument.connect([&](const App::Document& document) {
        if (&document == doc()) {
            checkLifetime();
        }
    });

    doc()->restore(savedPath.c_str());

    EXPECT_EQ(callbacks, 3);
    start.disconnect();
    files.disconnect();
    finish.disconnect();
    EXPECT_TRUE(app.closeDocument(name.c_str()));
}

TEST_F(DocumentTest, RestoreLifetimeUnwindsAfterCallbackException)
{
    auto& app = App::GetApplication();
    const std::string path = App::Application::getTempFileName();
    ASSERT_TRUE(doc()->saveAs(path.c_str()));
    const std::string savedPath = doc()->getFileName();
    Base::ScopeGuard removeFile([&] { std::remove(savedPath.c_str()); });
    auto connection = app.signalStartRestoreDocument.connect([&](const App::Document& document) {
        if (&document == doc()) {
            throw std::runtime_error("owned restore fault");
        }
    });

    EXPECT_THROW(doc()->restore(savedPath.c_str()), std::runtime_error);

    connection.disconnect();
    EXPECT_TRUE(app.closeDocument(doc()->getName()));
}

TEST_F(DocumentTest, SaveProtectsReceiverAndUnwindsAfterCallbackException)
{
    auto& app = App::GetApplication();
    const std::string path = App::Application::getTempFileName() + ".FCStd";
    Base::ScopeGuard removeFile([&] { std::remove(path.c_str()); });
    bool called = false;
    auto connection = doc()->signalStartSave.connect([&](const App::Document&, const std::string&) {
        called = true;
        auto& coordinator = App::RecomputeDetail::Coordinator::instance();
        const bool protectedNow = coordinator.isObserved(coordinator.bindDocument(doc()));
        EXPECT_TRUE(protectedNow);
        if (protectedNow) {
            EXPECT_FALSE(app.closeDocument(doc()->getName()));
        }
        throw std::runtime_error("owned save fault");
    });

    EXPECT_THROW(doc()->saveCopy(path.c_str()), std::runtime_error);

    EXPECT_TRUE(called);
    connection.disconnect();
    EXPECT_TRUE(app.closeDocument(doc()->getName()));
}

TEST_F(DocumentTest, NestedRestoreProtectsBothReceivers)
{
    auto& app = App::GetApplication();
    const std::string path = App::Application::getTempFileName();
    ASSERT_TRUE(doc()->saveAs(path.c_str()));
    const std::string savedPath = doc()->getFileName();
    auto* other = app.newDocument("NestedRestore");
    const std::string otherName = other->getName();
    const std::string name = doc()->getName();
    Base::ScopeGuard cleanup([&] {
        app.closeDocument(otherName.c_str());
        std::remove(savedPath.c_str());
    });
    bool nested = false;
    auto connection = app.signalStartRestoreDocument.connect([&](const App::Document& document) {
        if (&document == doc()) {
            other->restore(savedPath.c_str());
        }
        else if (&document == other) {
            nested = true;
            EXPECT_FALSE(app.closeDocument(name.c_str()));
            EXPECT_FALSE(app.closeDocument(otherName.c_str()));
        }
    });

    doc()->restore(savedPath.c_str());

    EXPECT_TRUE(nested);
    connection.disconnect();
    EXPECT_TRUE(app.closeDocument(otherName.c_str()));
    EXPECT_TRUE(app.closeDocument(name.c_str()));
}

TEST_F(DocumentTest, OpenProtectsGraphUntilFinalNotification)
{
    auto& app = App::GetApplication();
    const std::string path = App::Application::getTempFileName();
    ASSERT_TRUE(doc()->saveAs(path.c_str()));
    const std::string savedPath = doc()->getFileName();
    Base::ScopeGuard cleanup([&] { std::remove(savedPath.c_str()); });
    ASSERT_TRUE(app.closeDocument(doc()->getName()));
    std::string openedName;
    bool finalCalled = false;
    auto created = app.signalNewDocument.connect([&](const App::Document& document, bool) {
        openedName = document.getName();
        EXPECT_FALSE(app.closeDocument(openedName.c_str()));
    });
    auto start = app.signalStartRestoreDocument.connect([&](const App::Document&) {
        EXPECT_THROW(app.openDocument(savedPath.c_str()), Base::RuntimeError);
    });
    auto finish = app.signalFinishOpenDocument.connect([&] {
        finalCalled = true;
        EXPECT_FALSE(app.closeDocument(openedName.c_str()));
    });

    auto* opened = app.openDocument(savedPath.c_str());

    ASSERT_NE(opened, nullptr);
    EXPECT_TRUE(finalCalled);
    EXPECT_EQ(app.getDocument(openedName.c_str()), opened);
    created.disconnect();
    start.disconnect();
    finish.disconnect();
    EXPECT_TRUE(app.closeDocument(openedName.c_str()));
}

TEST_F(DocumentTest, NativeOperationObservationIncludesDetachedPropertyStorage)
{
    auto& coordinator = App::RecomputeDetail::Coordinator::instance();
    App::PropertyString detached;
    EXPECT_FALSE(coordinator.hasActiveOperations());
    {
        auto storage = coordinator.snapshotProperties({&detached});
        EXPECT_TRUE(coordinator.hasActiveOperations());
    }
    EXPECT_FALSE(coordinator.hasActiveOperations());
}

TEST_F(DocumentTest, NativeOperationObservationUnwindsWithPersistence)
{
    auto& coordinator = App::RecomputeDetail::Coordinator::instance();
    EXPECT_FALSE(coordinator.hasActiveOperations());
    {
        auto atomic = coordinator.atomic();
        EXPECT_TRUE(coordinator.hasActiveOperations());
    }
    EXPECT_FALSE(coordinator.hasActiveOperations());
    auto connection = doc()->signalStartSave.connect([&](const App::Document&, const std::string&) {
        EXPECT_TRUE(coordinator.hasActiveOperations());
    });
    const std::string path = App::Application::getTempFileName() + ".FCStd";
    Base::ScopeGuard cleanup([&] { std::remove(path.c_str()); });

    ASSERT_TRUE(doc()->saveCopy(path.c_str()));

    connection.disconnect();
    EXPECT_FALSE(coordinator.hasActiveOperations());
}

TEST_F(DocumentTest, RecomputeRejectsReentrantClose)
{
    auto& app = App::GetApplication();
    const std::string name = doc()->getName();
    bool called = false;
    auto connection = doc()->signalBeforeRecompute.connect([&](const App::Document& changed) {
        called = true;
        EXPECT_TRUE(changed.testStatus(App::Document::Recomputing));
        EXPECT_FALSE(changed.isClosable());
        EXPECT_FALSE(app.closeDocument(name.c_str()));
        EXPECT_EQ(app.getDocument(name.c_str()), doc());
    });

    doc()->recompute();

    EXPECT_TRUE(called);
    EXPECT_TRUE(doc()->isClosable());
    connection.disconnect();
    EXPECT_TRUE(app.closeDocument(name.c_str()));
}

TEST_F(DocumentTest, RecomputeRejectsCloseFromRecomputedObserver)
{
    auto& app = App::GetApplication();
    const std::string name = doc()->getName();
    bool called = false;
    auto connection = doc()->signalRecomputed.connect([&](const App::Document& changed,
                                                          const std::vector<App::DocumentObject*>&) {
        called = true;
        EXPECT_FALSE(changed.isClosable());
        EXPECT_FALSE(app.closeDocument(name.c_str()));
    });

    doc()->recompute();

    EXPECT_TRUE(called);
    EXPECT_EQ(app.getDocument(name.c_str()), doc());
    EXPECT_TRUE(doc()->isClosable());
    connection.disconnect();
}

TEST_F(DocumentTest, RecomputeRejectsCloseFromStableObserver)
{
    auto& app = App::GetApplication();
    const std::string name = doc()->getName();
    bool called = false;
    auto connection = doc()->signalBecameStable.connect([&](const App::Document& changed) {
        called = true;
        EXPECT_FALSE(changed.testStatus(App::Document::Recomputing));
        EXPECT_FALSE(changed.isClosable());
        EXPECT_FALSE(app.closeDocument(name.c_str()));
    });

    doc()->recompute();

    EXPECT_TRUE(called);
    EXPECT_EQ(app.getDocument(name.c_str()), doc());
    EXPECT_TRUE(doc()->isClosable());
    connection.disconnect();
}

TEST_F(DocumentTest, RecomputeProtectsOuterDocumentDuringNestedRecompute)
{
    auto& app = App::GetApplication();
    const std::string name = doc()->getName();
    const std::string nestedName = app.getUniqueDocumentName("nested_recompute");
    auto* nested = app.newDocument(nestedName.c_str(), "testUser");
    Base::ScopeGuard cleanup([&] { app.closeDocument(nestedName.c_str()); });
    bool called = false;
    auto nestedConnection = nested->signalBeforeRecompute.connect([&](const App::Document&) {
        called = true;
        EXPECT_FALSE(doc()->isClosable());
        EXPECT_FALSE(app.closeDocument(name.c_str()));
    });
    auto connection = doc()->signalBeforeRecompute.connect([&](const App::Document&) {
        nested->recompute();
    });

    doc()->recompute();

    EXPECT_TRUE(called);
    EXPECT_EQ(app.getDocument(name.c_str()), doc());
    EXPECT_TRUE(doc()->isClosable());
    EXPECT_TRUE(nested->isClosable());
    connection.disconnect();
    nestedConnection.disconnect();
}

TEST_F(DocumentTest, RecomputeCloseProtectionUnwindsAfterObserverException)
{
    auto connection = doc()->signalBeforeRecompute.connect([](const App::Document&) {
        throw std::runtime_error("recompute observer failed");
    });

    EXPECT_THROW(doc()->recompute(), std::runtime_error);

    EXPECT_FALSE(doc()->testStatus(App::Document::Recomputing));
    EXPECT_TRUE(doc()->isClosable());
    connection.disconnect();
    EXPECT_TRUE(App::GetApplication().closeDocument(doc()->getName()));
}

TEST_F(DocumentTest, CloseAllDocumentsReturnsWhenRecomputeIsActive)
{
    auto& app = App::GetApplication();
    const std::string name = doc()->getName();
    bool called = false;
    auto connection = doc()->signalBeforeRecompute.connect([&](const App::Document&) {
        called = true;
        app.closeAllDocuments();
        EXPECT_EQ(app.getDocument(name.c_str()), doc());
    });

    doc()->recompute();

    EXPECT_TRUE(called);
    EXPECT_TRUE(doc()->isClosable());
    connection.disconnect();
}

TEST_F(DocumentTest, SkippedRecomputeRejectsReentrantClose)
{
    auto& app = App::GetApplication();
    const std::string name = doc()->getName();
    doc()->setStatus(App::Document::SkipRecompute, true);
    bool called = false;
    auto connection = doc()->signalSkipRecompute.connect(
        [&](const App::Document& changed, const std::vector<App::DocumentObject*>&) {
            called = true;
            EXPECT_FALSE(changed.isClosable());
            EXPECT_FALSE(app.closeDocument(name.c_str()));
        }
    );

    doc()->recompute();

    EXPECT_TRUE(called);
    EXPECT_EQ(app.getDocument(name.c_str()), doc());
    EXPECT_TRUE(doc()->isClosable());
    connection.disconnect();
}

TEST_F(DocumentTest, FeatureRecomputeRejectsReentrantClose)
{
    auto& app = App::GetApplication();
    const std::string name = doc()->getName();
    auto* feature = doc()->addObject("App::FeaturePython", "Feature");
    bool called = false;
    auto connection = doc()->signalRecomputedObject.connect([&](const App::DocumentObject&) {
        called = true;
        EXPECT_FALSE(doc()->isClosable());
        EXPECT_FALSE(app.closeDocument(name.c_str()));
    });

    EXPECT_TRUE(doc()->recomputeFeature(feature));

    EXPECT_TRUE(called);
    EXPECT_EQ(app.getDocument(name.c_str()), doc());
    EXPECT_TRUE(doc()->isClosable());
    connection.disconnect();
}

TEST_F(DocumentTest, RecomputePreservesExplicitNonClosableStatus)
{
    doc()->setClosable(false);

    doc()->recompute();

    EXPECT_FALSE(doc()->isClosable());
    doc()->setClosable(true);
    EXPECT_TRUE(doc()->isClosable());
}

TEST_F(DocumentTest, RemovingExpressionOwnerClearsPropertyBacklinks)
{
    auto* source = doc()->addObject("App::FeaturePython", "Source");
    auto* thickness = static_cast<App::PropertyLength*>(
        source->addDynamicProperty("App::PropertyLength", "Thickness")
    );
    source->addDynamicProperty("App::PropertyLength", "Width");
    auto* removed = doc()->addObject("App::FeaturePython", "Removed");
    removed->addDynamicProperty("App::PropertyLength", "Length");
    removed->setExpression(
        App::ObjectIdentifier::parse(removed, "Length"),
        std::shared_ptr<App::Expression>(
            App::Expression::parse(removed, "Source.Thickness + Source.Width")
        )
    );
    auto* survivor = doc()->addObject("App::FeaturePython", "Survivor");
    auto* length = static_cast<App::PropertyLength*>(
        survivor->addDynamicProperty("App::PropertyLength", "Length")
    );
    survivor->setExpression(
        App::ObjectIdentifier::parse(survivor, "Length"),
        std::shared_ptr<App::Expression>(App::Expression::parse(survivor, "Source.Thickness"))
    );
    thickness->setValue(0.8);
    doc()->recompute();
    ASSERT_EQ(source->getInListProp().size(), 3);

    doc()->removeObject("Removed");

    // Assert before recomputing: a stale edge would dereference the deleted object.
    ASSERT_EQ(source->getInListProp().size(), 1);
    EXPECT_EQ(source->getInListProp().front().fromObj, survivor);
    for (double value : {0.81, 0.8}) {
        thickness->setValue(value);
        doc()->recompute();
        EXPECT_DOUBLE_EQ(length->getValue(), value);
    }
}

TEST_F(DocumentTest, UndoExpressionOwnerRemovalRestoresPropertyBacklinks)
{
    auto* source = doc()->addObject("App::FeaturePython", "Source");
    auto* thickness = static_cast<App::PropertyLength*>(
        source->addDynamicProperty("App::PropertyLength", "Thickness")
    );
    auto* target = doc()->addObject("App::FeaturePython", "Target");
    target->addDynamicProperty("App::PropertyLength", "Length");
    target->setExpression(
        App::ObjectIdentifier::parse(target, "Length"),
        std::shared_ptr<App::Expression>(App::Expression::parse(target, "Source.Thickness"))
    );
    doc()->recompute();
    ASSERT_EQ(source->getInListProp().size(), 1);

    doc()->openTransaction("Remove expression owner");
    doc()->removeObject("Target");
    doc()->commitTransaction();
    ASSERT_TRUE(source->getInListProp().empty());

    ASSERT_TRUE(doc()->undo());
    ASSERT_EQ(source->getInListProp().size(), 1);
    thickness->setValue(0.81);
    doc()->recompute();
    auto* restored = doc()->getObject("Target");
    ASSERT_NE(restored, nullptr);
    auto* length = static_cast<App::PropertyLength*>(restored->getPropertyByName("Length"));
    ASSERT_NE(length, nullptr);
    EXPECT_DOUBLE_EQ(length->getValue(), 0.81);

    ASSERT_TRUE(doc()->redo());
    ASSERT_TRUE(source->getInListProp().empty());
    thickness->setValue(0.8);
    doc()->recompute();
}


TEST_F(DocumentTest, addStringHasherIndicatesUnwrittenWhenNew)
{
    // Arrange
    App::StringHasherRef hasher(new App::StringHasher);

    // Act
    auto addResult = doc()->addStringHasher(hasher);

    // Assert
    EXPECT_TRUE(addResult.first);
    EXPECT_THAT(addResult.second, Ne(-1));
}

TEST_F(DocumentTest, addStringHasherIndicatesAlreadyWritten)
{
    // Arrange
    App::StringHasherRef hasher(new App::StringHasher);
    doc()->addStringHasher(hasher);

    // Act
    auto addResult = doc()->addStringHasher(hasher);

    // Assert
    EXPECT_FALSE(addResult.first);
}

TEST_F(DocumentTest, getStringHasherGivesExpectedHasher)
{
    // Arrange
    App::StringHasherRef hasher(new App::StringHasher);
    auto pair = doc()->addStringHasher(hasher);
    int index = pair.second;

    // Act
    auto foundHasher = doc()->getStringHasher(index);

    // Assert
    EXPECT_EQ(hasher, foundHasher);
}

TEST_F(DocumentTest, importObjectsRestoresSourceStringHasher)
{
    // Arrange
    auto& app = App::GetApplication();
    const std::string sourceName = app.getUniqueDocumentName("MergeSource");
    App::Document* source = app.newDocument(sourceName.c_str(), "testUser");
    App::StringHasherRef sourceHasher = source->getStringHasher();
    ASSERT_TRUE(sourceHasher);
    sourceHasher->setSaveAll(true);
    sourceHasher->getID("persisted element name");

    const std::string path = App::Application::getTempFileName();
    ASSERT_TRUE(source->saveAs(path.c_str()));
    const std::string savedPath = source->getFileName();
    app.closeDocument(sourceName.c_str());

    std::size_t restoredStringCount = 0;
    auto connection = doc()->signalFinishImportObjects.connect(
        [this, &restoredStringCount](const std::vector<App::DocumentObject*>&) {
            App::StringHasherRef importedHasher = doc()->getStringHasher(0);
            restoredStringCount = importedHasher ? importedHasher->size() : 0;
        }
    );

    // Act
    std::ifstream stream(savedPath, std::ios::in | std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    App::MergeDocuments merge(doc());
    merge.importObjects(stream);

    // Assert
    EXPECT_GT(restoredStringCount, 0);
    connection.disconnect();
    std::remove(savedPath.c_str());
}

// NOLINTEND(readability-magic-numbers)

TEST_F(DocumentTest, RemovingPartWithoutTransactionRemovesItsOriginDatums)
{
    auto* part = doc()->addObject<App::Part>("Part");
    auto* origin = static_cast<App::Origin*>(part->Origin.getValue());
    ASSERT_NE(origin, nullptr);
    std::vector<std::string> names {origin->getNameInDocument()};
    for (auto* datum : origin->OriginFeatures.getValues()) {
        names.emplace_back(datum->getNameInDocument());
    }
    EXPECT_NO_THROW(doc()->removeObject("Part"));
    EXPECT_EQ(doc()->getObject("Part"), nullptr);
    for (const auto& name : names) {
        EXPECT_EQ(doc()->getObject(name.c_str()), nullptr) << name;
    }
}

TEST_F(DocumentTest, RemovingPartOriginCanBeUndone)
{
    auto* part = doc()->addObject<App::Part>("Part");
    const auto originName = std::string(part->Origin.getValue()->getNameInDocument());
    const auto count = doc()->countObjects();
    doc()->openTransaction("Remove part");
    doc()->removeObject("Part");
    doc()->commitTransaction();
    EXPECT_EQ(doc()->countObjects(), 0);
    doc()->undo();
    EXPECT_EQ(doc()->countObjects(), count);
    auto* restored = dynamic_cast<App::Part*>(doc()->getObject("Part"));
    ASSERT_NE(restored, nullptr);
    auto* origin = dynamic_cast<App::Origin*>(restored->Origin.getValue());
    ASSERT_NE(origin, nullptr);
    EXPECT_EQ(std::string(origin->getNameInDocument()), originName);
    EXPECT_EQ(origin->OriginFeatures.getSize(), 7);
    EXPECT_NO_THROW(doc()->recompute());
}
