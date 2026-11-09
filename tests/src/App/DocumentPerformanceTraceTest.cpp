// SPDX-License-Identifier: LGPL-2.1-or-later
#include <gtest/gtest.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <thread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ElementMap.h>
#include <App/PropertyFile.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/Writer.h>
#include <src/App/InitApplication.h>
#include "DocumentPerformanceTrace.h"

namespace Trace = App::DocumentPerformanceTrace;
namespace
{
class TraceTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
    void SetUp() override
    {
        ASSERT_TRUE(Trace::begin());
    }
    void TearDown() override
    {
        EXPECT_TRUE(Trace::end());
    }
};
}  // namespace
TEST_F(TraceTest, BeginEndAndDisabledScopes)
{
    EXPECT_FALSE(Trace::begin());
    {
        Trace::Scope save(Trace::Scope::Kind::Save);
        EXPECT_FALSE(Trace::begin());
        EXPECT_FALSE(Trace::end());
        EXPECT_EQ(Trace::counters().saveActive, 1);
        save.complete(true);
        save.complete();
    }
    EXPECT_EQ(Trace::counters().saveCalls, 1);
    EXPECT_EQ(Trace::counters().saveCompleted, 1);
    EXPECT_EQ(Trace::counters().saveEmpty, 1);
    EXPECT_EQ(Trace::counters().saveActive, 0);
    ASSERT_TRUE(Trace::end());
    EXPECT_FALSE(Trace::end());
    {
        Trace::Scope disabled(Trace::Scope::Kind::Map);
    }
    EXPECT_EQ(Trace::counters().mapCalls, 0);
    ASSERT_TRUE(Trace::begin());
    EXPECT_EQ(Trace::counters().saveCalls, 0);
}
TEST_F(TraceTest, NestedSameKindOnlyTimesOutermost)
{
    for (const auto kind : {Trace::Scope::Kind::Save, Trace::Scope::Kind::Map}) {
        Trace::Scope outer(kind);
        {
            Trace::Scope inner(kind);
            inner.complete();
        }
        const auto c = Trace::counters();
        EXPECT_EQ(kind == Trace::Scope::Kind::Save ? c.saveNanoseconds : c.mapNanoseconds, 0);
        EXPECT_EQ(kind == Trace::Scope::Kind::Save ? c.saveActive : c.mapActive, 1);
        outer.complete();
    }
    const auto c = Trace::counters();
    EXPECT_EQ(c.saveCalls, 2);
    EXPECT_EQ(c.mapCalls, 2);
    EXPECT_EQ(c.saveCompleted, 2);
    EXPECT_EQ(c.mapCompleted, 2);
    EXPECT_GT(c.saveNanoseconds, 0);
    EXPECT_GT(c.mapNanoseconds, 0);
    EXPECT_EQ(c.saveActive + c.mapActive, 0);
}
TEST_F(TraceTest, CrossKindAndExceptionOutcomes)
{
    {
        Trace::Scope save(Trace::Scope::Kind::Save);
        {
            Trace::Scope map(Trace::Scope::Kind::Map);
            Trace::tokens(7);
            Trace::record();
            map.complete();
        }
        EXPECT_EQ(Trace::counters().saveNanoseconds, 0);
    }
    const auto c = Trace::counters();
    EXPECT_EQ(c.saveFailed, 1);
    EXPECT_EQ(c.mapCompleted, 1);
    EXPECT_EQ(c.tokenBuffers, 1);
    EXPECT_EQ(c.tokenBytes, 7);
    EXPECT_EQ(c.mapRecords, 1);
    EXPECT_EQ(c.saveActive + c.mapActive, 0);
}
TEST_F(TraceTest, ThreadLocalIsolation)
{
    Trace::Counters other;
    Trace::Scope outer(Trace::Scope::Kind::Save);
    std::thread worker([&] {
        EXPECT_EQ(Trace::counters().saveCalls, 0);
        EXPECT_TRUE(Trace::begin());
        {
            Trace::Scope map(Trace::Scope::Kind::Map);
            Trace::tokens(9);
            map.complete();
        }
        EXPECT_TRUE(Trace::end());
        other = Trace::counters();
    });
    worker.join();
    outer.complete();
    EXPECT_EQ(other.mapCalls, 1);
    EXPECT_EQ(other.saveCalls, 0);
    EXPECT_EQ(Trace::counters().mapCalls, 0);
    EXPECT_EQ(Trace::counters().saveCalls, 1);
}
TEST_F(TraceTest, RealMapsCountRecordsAndRejectPartialCoverage)
{
    std::stringstream good(
        "997501 PostfixCount 0\nMapCount 1\nElementMap 1 997501 1\nFace\nChildCount 1\n1 0 1 0 0 "
        "Child 0\nNameCount 1\n;Raw.0 0\nEndMap\n"
    );
    auto first = std::make_shared<Data::ElementMap>()->restore({}, good);
    ASSERT_TRUE(first);
    auto c = Trace::counters();
    EXPECT_EQ(c.mapCalls, 1);
    EXPECT_EQ(c.mapCompleted, 1);
    EXPECT_EQ(c.mapFailed, 0);
    EXPECT_EQ(c.mapRecords, 1);
    EXPECT_EQ(c.childRecords, 1);
    EXPECT_EQ(c.tokenBuffers, 1);
    EXPECT_EQ(c.tokenBytes, 6);
    std::stringstream bad(
        "997502 PostfixCount 0\nMapCount 1\nElementMap 1 997502 1\nFace\nChildCount 0\nNameCount "
        "1\n;Raw.0 0\nTruncated\n"
    );
    EXPECT_THROW(std::make_shared<Data::ElementMap>()->restore({}, bad), Base::RuntimeError);
    c = Trace::counters();
    EXPECT_EQ(c.mapCalls, 2);
    EXPECT_EQ(c.mapCompleted, 1);
    EXPECT_EQ(c.mapFailed, 1);
    EXPECT_EQ(c.mapRecords, 2);
    EXPECT_EQ(c.mapActive, 0);
    // A caller must reject mapFailed>0; partial counters are never successful coverage.
    EXPECT_FALSE(c.mapFailed == 0 && c.mapRecords > 0);
}
TEST_F(TraceTest, DuplicateInputIdsRetainObservedPublicParserBehavior)
{
    for (int i = 0; i < 2; ++i) {
        std::stringstream input(
            "997503 PostfixCount 0\nMapCount 1\nElementMap 1 997503 1\nFace\nChildCount "
            "0\nNameCount 1\n;Raw.0 0\nEndMap\n"
        );
        ASSERT_TRUE(std::make_shared<Data::ElementMap>()->restore({}, input));
    }
    // The current source never populates its id-cache, so public duplicate input
    // still executes two parses. Do not invent a cache hit merely for this probe.
    EXPECT_EQ(Trace::counters().mapCalls, 2);
    EXPECT_EQ(Trace::counters().mapCompleted, 2);
    EXPECT_EQ(Trace::counters().mapRecords, 2);
}
TEST_F(TraceTest, CAbiHasOnlyNumericCounters)
{
    Trace::tokens(100);
    Trace::record();
    EXPECT_EQ(Trace::counters().tokenBuffers, 0);
    ASSERT_TRUE(fc_document_perf_end());
    const char* raw = fc_document_perf_snapshot();
    ASSERT_NE(raw, nullptr);
    const auto doc = QJsonDocument::fromJson(raw);
    ASSERT_TRUE(doc.isObject());
    const auto obj = doc.object();
    EXPECT_EQ(obj.size(), 15);
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        EXPECT_TRUE(it.value().isDouble());
    }
    EXPECT_EQ(std::string(raw).find('/'), std::string::npos);
    ASSERT_TRUE(fc_document_perf_begin());
}
namespace
{
class SaveTraceTest: public TraceTest
{
protected:
    void SetUp() override
    {
        TraceTest::SetUp();
        doc = App::GetApplication().newDocument("SaveTraceTest");
        auto* object = doc->addObject("App::VarSet", "Vars");
        property = static_cast<App::PropertyFileIncluded*>(
            object->addDynamicProperty("App::PropertyFileIncluded", "File")
        );
    }
    void TearDown() override
    {
        App::GetApplication().closeDocument(doc->getName());
        TraceTest::TearDown();
    }
    void payload(std::size_t size)
    {
        ASSERT_TRUE(directory.isValid());
        source = (directory.path() + "/payload.bin").toStdString();
        std::ofstream stream(source, std::ios::binary);
        std::string bytes(size, '\0');
        for (std::size_t i = 0; i < size; ++i) {
            bytes[i] = static_cast<char>(i % 251);
        }
        stream.write(bytes.data(), bytes.size());
        stream.close();
        property->setValue(source.c_str());
        expected = std::move(bytes);
    }
    App::Document* doc = nullptr;
    App::PropertyFileIncluded* property = nullptr;
    QTemporaryDir directory;
    std::string source, expected;
};
}  // namespace
TEST_F(SaveTraceTest, EmptyPropertyExplicitlyCountsNoPayload)
{
    Base::StringWriter writer;
    property->SaveDocFile(writer);
    const auto c = Trace::counters();
    EXPECT_EQ(c.saveCalls, 1);
    EXPECT_EQ(c.saveEmpty, 1);
    EXPECT_EQ(c.saveCompleted, 1);
    EXPECT_EQ(c.saveFailed, 0);
    EXPECT_EQ(c.saveActive, 0);
}
TEST_F(SaveTraceTest, BinaryMultibufferSaveMatchesBytes)
{
    payload(131089);
    Base::StringWriter writer;
    property->SaveDocFile(writer);
    EXPECT_EQ(writer.getString(), expected);
    const auto c = Trace::counters();
    EXPECT_EQ(c.saveCalls, 1);
    EXPECT_EQ(c.saveCompleted, 1);
    EXPECT_EQ(c.saveFailed, 0);
    EXPECT_EQ(c.saveActive, 0);
}
TEST_F(SaveTraceTest, ZeroByteFileIsSuccessfulNonemptyProperty)
{
    payload(0);
    Base::StringWriter writer;
    property->SaveDocFile(writer);
    EXPECT_TRUE(writer.getString().empty());
    const auto c = Trace::counters();
    EXPECT_EQ(c.saveCalls, 1);
    EXPECT_EQ(c.saveCompleted, 1);
    EXPECT_EQ(c.saveEmpty, 0);
    EXPECT_EQ(c.saveActive, 0);
}
TEST_F(SaveTraceTest, MissingFileRecordsFailure)
{
    payload(9);
    std::filesystem::remove(property->getValue());
    Base::StringWriter writer;
    EXPECT_THROW(property->SaveDocFile(writer), Base::FileSystemError);
    EXPECT_EQ(Trace::counters().saveFailed, 1);
    EXPECT_EQ(Trace::counters().saveActive, 0);
}
TEST_F(SaveTraceTest, OutputFailureRecordsFailure)
{
    payload(131089);
    Base::StringWriter writer;
    writer.Stream().setstate(std::ios::badbit);
    EXPECT_THROW(property->SaveDocFile(writer), Base::FileSystemError);
    EXPECT_EQ(Trace::counters().saveFailed, 1);
    EXPECT_EQ(Trace::counters().saveCompleted, 0);
    EXPECT_EQ(Trace::counters().saveActive, 0);
}
#if defined(__linux__)
TEST_F(SaveTraceTest, DirectoryReadFailsAfterSuccessfulOpen)
{
    payload(8);
    const std::string target = property->getValue();
    std::filesystem::remove(target);
    std::filesystem::create_directory(target);
    {
        Base::ifstream input(Base::FileInfo(target), std::ios::binary);
        ASSERT_TRUE(input);
        char value;
        input.get(value);
        ASSERT_TRUE(input.bad());
    }
    Base::StringWriter writer;
    EXPECT_THROW(property->SaveDocFile(writer), Base::FileSystemError);
    EXPECT_EQ(Trace::counters().saveFailed, 1);
    EXPECT_EQ(Trace::counters().saveActive, 0);
    std::filesystem::remove(target);
}
#endif
