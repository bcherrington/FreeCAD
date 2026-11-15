// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <filesystem>
#include <memory>
#include <tuple>

#include <gtest/gtest.h>
#include <src/App/InitApplication.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/ElementNamingUtils.h>
#include <Mod/Part/App/FeatureChamfer.h>
#include <Mod/Part/App/FeatureFillet.h>
#include <Mod/Part/App/FeaturePartBox.h>

#include "PartTestHelpers.h"

namespace
{
// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
class DressUpMissingEdgesTest
    : public ::testing::TestWithParam<std::tuple<const char*, std::vector<int>, bool>>
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        auto& app = App::GetApplication();
        document = app.newDocument(app.getUniqueDocumentName("MissingEdges").c_str());
        auto box = document->addObject<Part::Box>("Box");
        box->Length.setValue(10);
        box->Width.setValue(20);
        box->Height.setValue(30);
        feature = static_cast<Part::FilletBase*>(
            document->addObject(std::get<0>(GetParam()), "DressUp")
        );
        feature->Base.setValue(box);
        feature->Edges.setValues(validEdges());
        document->recompute();
        ASSERT_FALSE(feature->Shape.getValue().IsNull());
        ASSERT_FALSE(feature->isError());
    }

    void TearDown() override
    {
        if (document) {
            App::GetApplication().closeDocument(document->getName());
        }
        if (!path.empty()) {
            std::filesystem::remove(path);
        }
    }

    static std::vector<Part::FilletElement> validEdges()
    {
        return {{1, 0.2, 0.2}, {2, 0.3, 0.3}, {3, 0.4, 0.4}};
    }

    std::string setMissingReferences()
    {
        const auto& missing = std::get<1>(GetParam());
        auto edges = validEdges();
        // Equal entries exercise the original remove/erase aliasing case too.
        if (missing == std::vector<int> {0, 1}) {
            edges[1] = edges[0];
        }
        feature->Edges.setValues(edges);
        auto refs = feature->EdgeLinks.getSubValues();
        auto shadows = feature->EdgeLinks.getShadowSubs();
        for (int index : missing) {
            const auto name = "?Edge" + std::to_string(edges[index].edgeid);
            if (std::get<2>(GetParam())) {
                shadows[index].oldName = name;
            }
            else {
                refs[index] = name;
                shadows[index].newName = name;
                shadows[index].oldName = name;
            }
        }
        feature->EdgeLinks.setValue(feature->Base.getValue(), refs, std::move(shadows));
        const auto& actual = feature->EdgeLinks.getShadowSubs();
        const auto values = feature->EdgeLinks.getSubValues(true);
        std::string expected;
        for (size_t i = 0; i < actual.size(); ++i) {
            const auto& ref = actual[i].newName.empty() ? values[i] : actual[i].newName;
            if (Data::hasMissingElement(ref.c_str())
                || Data::hasMissingElement(actual[i].oldName.c_str())) {
                expected += "Missing edge link: " + ref + "\n";
            }
        }
        EXPECT_EQ(std::count(expected.begin(), expected.end(), '\n'), missing.size());
        return expected;
    }

    void expectFailure(const std::string& expected)
    {
        auto result = feature->execute();
        ASSERT_NE(result, App::DocumentObject::StdReturn);
        std::unique_ptr<App::DocumentObjectExecReturn> error(result);
        EXPECT_EQ(error->Why, expected);
    }

    App::Document* document = nullptr;
    Part::FilletBase* feature = nullptr;
    std::string path;
};

TEST_P(DressUpMissingEdgesTest, ReportsEveryMissingReferenceWithoutChangingInputsOrShape)
{
    const auto originalShape = feature->Shape.getValue();
    const auto volume = PartTestHelpers::getVolume(originalShape);
    const auto expected = setMissingReferences();
    const auto edges = feature->Edges.getValues();
    const auto refs = feature->EdgeLinks.getSubValues();
    const auto shadows = feature->EdgeLinks.getShadowSubs();

    for (int attempt = 0; attempt < 2; ++attempt) {
        expectFailure(expected);
        EXPECT_EQ(feature->Edges.getValues(), edges);
        EXPECT_EQ(feature->EdgeLinks.getSubValues(), refs);
        EXPECT_EQ(feature->EdgeLinks.getShadowSubs(), shadows);
        EXPECT_TRUE(feature->Shape.getValue().IsSame(originalShape));
        EXPECT_DOUBLE_EQ(PartTestHelpers::getVolume(feature->Shape.getValue()), volume);
    }

    // Repair the references through the ordinary Edges property synchronization.
    feature->Edges.setValues(validEdges());
    document->recompute();
    EXPECT_FALSE(feature->isError());
    EXPECT_NEAR(PartTestHelpers::getVolume(feature->Shape.getValue()), volume, 1e-8);
}

TEST_P(DressUpMissingEdgesTest, MissingReferencesSurviveUndoAndRoundTripThenRepair)
{
    const auto volume = PartTestHelpers::getVolume(feature->Shape.getValue());
    document->openTransaction("Break dress-up references");
    const auto expected = setMissingReferences();
    document->commitTransaction();
    const auto brokenEdges = feature->Edges.getValues();
    expectFailure(expected);

    ASSERT_TRUE(document->undo());
    document->recompute();
    EXPECT_FALSE(feature->isError());
    EXPECT_EQ(feature->Edges.getValues(), validEdges());
    EXPECT_NEAR(PartTestHelpers::getVolume(feature->Shape.getValue()), volume, 1e-8);
    ASSERT_TRUE(document->redo());
    expectFailure(expected);
    EXPECT_EQ(feature->Edges.getValues(), brokenEdges);

    path = App::Application::getTempFileName() + ".FCStd";
    document->saveAs(path.c_str());
    auto& app = App::GetApplication();
    const std::string name = document->getName();
    app.closeDocument(name.c_str());
    document = nullptr;
    document = app.openDocument(path.c_str());
    ASSERT_NE(document, nullptr);
    feature = dynamic_cast<Part::FilletBase*>(document->getObject("DressUp"));
    ASSERT_NE(feature, nullptr);
    EXPECT_EQ(feature->Edges.getValues(), brokenEdges);
    expectFailure(expected);
    feature->Edges.setValues(validEdges());
    document->recompute();
    EXPECT_FALSE(feature->isError());
    EXPECT_NEAR(PartTestHelpers::getVolume(feature->Shape.getValue()), volume, 1e-8);
}

INSTANTIATE_TEST_SUITE_P(
    PartDressUp,
    DressUpMissingEdgesTest,
    ::testing::Combine(
        ::testing::Values("Part::Fillet", "Part::Chamfer"),
        ::testing::Values(
            std::vector<int> {0},
            std::vector<int> {1},
            std::vector<int> {2},
            std::vector<int> {0, 2},
            std::vector<int> {0, 1},
            std::vector<int> {0, 1, 2}
        ),
        ::testing::Bool()
    )
);
// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
}  // namespace
