// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <fstream>
#include <stdexcept>

#include "App/Application.h"
#include "App/Document.h"
#include "App/MergeDocuments.h"
#include "App/StringHasher.h"
#include "Base/Tools.h"
#include "Base/Writer.h"
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
