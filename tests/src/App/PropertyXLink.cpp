// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <QDir>
#include <QTemporaryDir>
#include <sstream>
#include <zipios++/zipinputstream.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/private/RecomputeAdmission.h>
#include <Base/Writer.h>
#include <Base/Reader.h>

#include "InitApplication.h"

class PropertyXLinkSaveTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        ASSERT_TRUE(directory.isValid());
        ASSERT_TRUE(QDir(directory.path()).mkpath("parts"));
        ASSERT_TRUE(QDir(directory.path()).mkpath("copies"));
        owner = App::GetApplication().newDocument("XLinkOwner");
        target = App::GetApplication().newDocument("XLinkTarget");
        owner->FileName.setValue(path("Root.FCStd"));
        target->FileName.setValue(path("parts/Definitions.FCStd"));
        object = owner->addObject("App::DocumentObject", "Owner");
        dependency = target->addObject("App::DocumentObject", "Definition");
        link = static_cast<App::PropertyXLink*>(
            object->addDynamicProperty("App::PropertyXLink", "Reference")
        );
        link->setValue(dependency);
    }

    void TearDown() override
    {
        if (owner) {
            App::GetApplication().closeDocument(owner->getName());
        }
        if (target) {
            App::GetApplication().closeDocument(target->getName());
        }
    }

    std::string path(const char* relative) const
    {
        return QDir(directory.path()).filePath(QString::fromUtf8(relative)).toStdString();
    }

    std::string serialized(const char* mode = nullptr) const
    {
        Base::StringWriter writer;
        if (mode) {
            writer.setMode(mode);
        }
        link->Save(writer);
        return writer.getString();
    }

    void expectFile(const std::string& file, const char* mode = nullptr) const
    {
        EXPECT_NE(serialized(mode).find("file=\"" + file + "\""), std::string::npos)
            << serialized(mode);
    }

    QTemporaryDir directory;
    App::Document* owner {};
    App::Document* target {};
    App::DocumentObject* object {};
    App::DocumentObject* dependency {};
    App::PropertyXLink* link {};
};

TEST_F(PropertyXLinkSaveTest, LoadedDependencyUsesOwnerRelativePath)
{
    expectFile("parts/Definitions.FCStd");
    // Serialization must not change the live dependency cache's identity.
    EXPECT_EQ(std::string(link->getDocumentPath()), path("parts/Definitions.FCStd"));
    EXPECT_EQ(link->getValue(), dependency);
}

TEST_F(PropertyXLinkSaveTest, OwnerSaveAsRebasesPath)
{
    ASSERT_TRUE(owner->saveAs(path("copies/Renamed.FCStd").c_str()));
    expectFile("../parts/Definitions.FCStd");
    EXPECT_EQ(link->getValue(), dependency);
}

TEST_F(PropertyXLinkSaveTest, TargetSaveAsUpdatesDependencyPath)
{
    ASSERT_TRUE(target->saveAs(path("parts/Renamed.FCStd").c_str()));
    expectFile("parts/Renamed.FCStd");
    EXPECT_EQ(link->getValue(), dependency);
}

TEST_F(PropertyXLinkSaveTest, ClosedDependencyStillUsesRelativePath)
{
    App::GetApplication().closeDocument(target->getName());
    target = nullptr;
    EXPECT_EQ(link->getValue(), nullptr);
    expectFile("parts/Definitions.FCStd");
}

TEST_F(PropertyXLinkSaveTest, SaveCopyRetainsAbsoluteDependency)
{
    expectFile(path("parts/Definitions.FCStd"), "SaveCopy");
    EXPECT_EQ(link->getValue(), dependency);
}

TEST_F(PropertyXLinkSaveTest, InternalLinkHasNoExternalPath)
{
    link->setValue(owner->addObject("App::DocumentObject", "Local"));
    expectFile("");
    expectFile("", "SaveCopy");
}

TEST_F(PropertyXLinkSaveTest, ClearedLinkHasNoExternalPath)
{
    link->setValue(nullptr);
    expectFile("");
}

TEST_F(PropertyXLinkSaveTest, ExportOwnerKeepsAbsoluteDependency)
{
    std::stringstream archive;
    owner->exportObjects({object}, archive);
    zipios::ZipInputStream stream(archive);
    Base::XMLReader reader("Document.xml", stream);
    reader.readElement("XLink");
    EXPECT_EQ(reader.getAttribute<std::string>("file"), path("parts/Definitions.FCStd"));
    expectFile("parts/Definitions.FCStd");
}

TEST_F(PropertyXLinkSaveTest, ExportTogetherKeepsInternalReference)
{
    std::stringstream archive;
    owner->exportObjects({object, dependency}, archive);
    zipios::ZipInputStream stream(archive);
    Base::XMLReader reader("Document.xml", stream);
    reader.readElement("XLink");
    EXPECT_STREQ(reader.getAttribute<const char*>("file", ""), "");
    EXPECT_FALSE(reader.getAttribute<std::string>("name").empty());
}

TEST_F(PropertyXLinkSaveTest, RemovingUnrelatedObjectDoesNotReserveExternalSource)
{
    owner->addObject("App::DocumentObject", "Independent");
    {
        // Keep an unrelated external definition leased by a different operation.
        auto claim = App::RecomputeDetail::Coordinator::instance().claimDocument(target, dependency);
        EXPECT_NO_THROW(owner->removeObject("Independent"));
    }
    EXPECT_EQ(owner->getObject("Independent"), nullptr);
    EXPECT_EQ(link->getValue(), dependency);
}

TEST_F(PropertyXLinkSaveTest, RemovingTargetClearsHiddenAndExternalReceivers)
{
    auto* local = owner->addObject("App::DocumentObject", "Local");
    auto* keep = owner->addObject("App::DocumentObject", "Keep");
    auto* hidden = static_cast<App::PropertyLinkHidden*>(
        object->addDynamicProperty("App::PropertyLinkHidden", "HiddenReference")
    );
    auto* list = static_cast<App::PropertyLinkList*>(
        object->addDynamicProperty("App::PropertyLinkList", "LocalReferences")
    );
    hidden->setValue(local);
    list->setValues({local, keep});
    EXPECT_NO_THROW(owner->removeObject("Local"));
    EXPECT_EQ(hidden->getValue(), nullptr);
    ASSERT_EQ(list->getValues().size(), 1);
    EXPECT_EQ(list->getValues().front(), keep);
    EXPECT_EQ(link->getValue(), dependency);

    EXPECT_NO_THROW(target->removeObject("Definition"));
    dependency = nullptr;
    EXPECT_EQ(link->getValue(), nullptr);
}
