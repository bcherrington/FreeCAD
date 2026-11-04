// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <stdexcept>

#include "App/Application.h"
#include "App/Document.h"
#include "App/MergeDocuments.h"
#include "App/StringHasher.h"
#include "Base/Interpreter.h"
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

class RecomputeCloseAllOrderTest: public DocumentTest,
                                  public ::testing::WithParamInterface<std::size_t>
{
};

TEST_P(RecomputeCloseAllOrderTest, StopsAtProtectedDocumentAndClosesRemainingDocumentsLater)
{
    auto& app = App::GetApplication();
    const std::string protectedName = doc()->getName();
    const auto protectedIndex = GetParam();
    const std::array<const char*, 3> firstNames {"zz_close_first", "aa_close_first", "aa_close_first"};
    const std::array<const char*, 3> secondNames {"zz_close_second", "zz_close_second", "aa_close_second"};
    const std::array<std::string, 2> otherNames {
        app.getUniqueDocumentName(firstNames[protectedIndex]),
        app.getUniqueDocumentName(secondNames[protectedIndex])
    };
    Base::ScopeGuard cleanup([&] {
        for (const auto& name : otherNames) {
            app.closeDocument(name.c_str());
        }
    });
    for (const auto& name : otherNames) {
        app.newDocument(name.c_str(), "testUser");
    }
    ASSERT_EQ(app.getDocuments().size(), 3);
    std::array<std::string, 3> orderedNames {protectedName, otherNames[0], otherNames[1]};
    std::sort(orderedNames.begin(), orderedNames.end());
    ASSERT_EQ(orderedNames[protectedIndex], protectedName);

    bool called = false;
    auto connection = doc()->signalBeforeRecompute.connect([&](const App::Document&) {
        called = true;
        app.closeAllDocuments();
        for (std::size_t index = 0; index < orderedNames.size(); ++index) {
            EXPECT_EQ(app.getDocument(orderedNames[index].c_str()) != nullptr, index >= protectedIndex);
        }
    });

    doc()->recompute();

    EXPECT_TRUE(called);
    EXPECT_TRUE(doc()->isClosable());
    connection.disconnect();
    app.closeAllDocuments();
    for (const auto& name : orderedNames) {
        EXPECT_EQ(app.getDocument(name.c_str()), nullptr);
    }
}

INSTANTIATE_TEST_SUITE_P(
    RecomputeClose,
    RecomputeCloseAllOrderTest,
    ::testing::Values(std::size_t {0}, std::size_t {1}, std::size_t {2})
);

class PythonRecomputeCloseTest: public DocumentTest, public ::testing::WithParamInterface<const char*>
{
};

TEST_P(PythonRecomputeCloseTest, RaisesDuringObserverAndClosesAfterRecompute)
{
    const std::string script = std::string("phase = '") + GetParam() + "'\n" + R"PY(
import FreeCAD as App

def check_python_close_contract():
    document = App.newDocument()
    name = document.Name
    observations = []

    class Observer:
        def check(self, changed, callback_phase):
            if changed.Name != name or callback_phase != phase:
                return
            result = {"closable": changed.isClosable(), "runtime_error": False}
            try:
                App.closeDocument(name)
            except RuntimeError:
                result["runtime_error"] = True
            result["alive"] = name in App.listDocuments()
            observations.append(result)

        def slotBeforeRecomputeDocument(self, changed):
            self.check(changed, "before")

        def slotRecomputedDocument(self, changed):
            self.check(changed, "recomputed")

    observer = Observer()
    App.addDocumentObserver(observer)
    try:
        document.recompute()
        expected = {"closable": False, "runtime_error": True, "alive": True}
        assert observations == [expected], observations
        assert document.isClosable(), "recompute close protection did not unwind"
    finally:
        App.removeDocumentObserver(observer)
        if name in App.listDocuments():
            App.closeDocument(name)
    assert name not in App.listDocuments(), "ordinary close after recompute failed"

check_python_close_contract()
)PY";
    ASSERT_NO_THROW(Base::Interpreter().runString(script.c_str()));
}

INSTANTIATE_TEST_SUITE_P(
    RecomputeClose,
    PythonRecomputeCloseTest,
    ::testing::Values("before", "recomputed")
);


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
