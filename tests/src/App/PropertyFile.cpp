// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>
#include <sstream>
#include <string>

#include <Base/Exception.h>
#include <Base/Reader.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyFile.h>

#include "InitApplication.h"

namespace
{
void restoreFileIncluded(App::PropertyFileIncluded& prop, const std::string& fileIncludedElement)
{
    std::string xml = "<?xml version='1.0' encoding='utf-8'?>\n";
    xml += "<Property name='File' type='App::PropertyFileIncluded'>\n";
    xml += fileIncludedElement;
    xml += "\n</Property>\n";

    std::stringstream data(xml);
    Base::XMLReader reader("Document.xml", data);
    prop.Restore(reader);
}
}  // namespace

// Regression tests for GHSA-5vqh-3v38-jw2r: a crafted .FCStd must not be able to escape the
// document transient directory through the "file" or "data" attributes of a FileIncluded element.
class PropertyFileIncludedTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
    void SetUp() override
    {
        _doc = App::GetApplication().newDocument("PropertyFileIncludedTest");
        _object = _doc->addObject("App::VarSet", "VarSet");
        _property = static_cast<App::PropertyFileIncluded*>(
            _object->addDynamicProperty("App::PropertyFileIncluded", "File")
        );
    }
    void TearDown() override
    {
        App::GetApplication().closeDocument(_doc->getName());
    }
    App::Document* _doc {nullptr};
    App::DocumentObject* _object {nullptr};
    App::PropertyFileIncluded* _property {nullptr};
};

TEST_F(PropertyFileIncludedTest, rejectsParentTraversalInFileAttribute)
{
    EXPECT_THROW(
        restoreFileIncluded(*_property, "<FileIncluded file='../../../.bashrc'/>"),
        Base::FileException
    );
    EXPECT_TRUE(std::string(_property->getValue()).empty());
}

TEST_F(PropertyFileIncludedTest, rejectsBackslashTraversalInFileAttribute)
{
    EXPECT_THROW(
        restoreFileIncluded(*_property, "<FileIncluded file='..\\..\\.bashrc'/>"),
        Base::FileException
    );
    EXPECT_TRUE(std::string(_property->getValue()).empty());
}

TEST_F(PropertyFileIncludedTest, rejectsAbsolutePathInFileAttribute)
{
    EXPECT_THROW(
        restoreFileIncluded(*_property, "<FileIncluded file='/etc/passwd'/>"),
        Base::FileException
    );
    EXPECT_TRUE(std::string(_property->getValue()).empty());
}

TEST_F(PropertyFileIncludedTest, rejectsSubdirectoryInFileAttribute)
{
    EXPECT_THROW(
        restoreFileIncluded(*_property, "<FileIncluded file='sub/payload'/>"),
        Base::FileException
    );
    EXPECT_TRUE(std::string(_property->getValue()).empty());
}

TEST_F(PropertyFileIncludedTest, rejectsDotDotInFileAttribute)
{
    EXPECT_THROW(restoreFileIncluded(*_property, "<FileIncluded file='..'/>"), Base::FileException);
    EXPECT_TRUE(std::string(_property->getValue()).empty());
}

TEST_F(PropertyFileIncludedTest, rejectsParentTraversalInDataAttribute)
{
    EXPECT_THROW(
        restoreFileIncluded(*_property, "<FileIncluded data='../../../.bashrc'/>"),
        Base::FileException
    );
    EXPECT_TRUE(std::string(_property->getValue()).empty());
}

TEST_F(PropertyFileIncludedTest, acceptsPlainBasename)
{
    // A legitimate basename (as always produced by Save()) must still restore and resolve inside
    // the document transient directory.
    EXPECT_NO_THROW(restoreFileIncluded(*_property, "<FileIncluded file='PartShape.brp'/>"));

    // getDocTransientPath() normalizes backslashes to forward slashes, so normalize the expected
    // transient directory the same way before comparing.
    std::string transientDir = _doc->TransientDir.getValue();
    std::ranges::replace(transientDir, '\\', '/');
    EXPECT_EQ(std::string(_property->getValue()), transientDir + "/PartShape.brp");
}

#ifdef FC_TEST_INCLUDED_FILE_IO_FAULTS
# include "PropertyFileIOFaults.h"

# include <Base/Console.h>
# include <Base/FileInfo.h>
# include <Base/Persistence.h>
# include <Base/Writer.h>
# include <QCryptographicHash>
# include <QTemporaryDir>
# include <vector>
# include <filesystem>
# include <fstream>
# include <functional>
# include <mutex>
# include <tuple>

namespace
{
std::string includedBytes(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void writeIncludedBytes(const std::string& filename, const std::string& bytes)
{
    std::ofstream file(filename, std::ios::binary);
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(file.good());
}

std::string binaryPayload(std::size_t size)
{
    std::string data(size, '\0');
    for (std::size_t i = 0; i < size; ++i) {
        data[i] = static_cast<char>(i * 37 % 256);
    }
    return data;
}

class PayloadHook: public Base::Persistence
{
public:
    std::function<void(Base::Reader&)> read;
    unsigned int getMemSize() const override
    {
        return 0;
    }
    void Save(Base::Writer&) const override
    {}
    void Restore(Base::XMLReader&) override
    {}
    void RestoreDocFile(Base::Reader& reader) override
    {
        read(reader);
    }
};

class FailingInput: public std::streambuf
{
public:
    FailingInput(Base::Reader& source, std::streamsize remaining)
        : source(source)
        , remaining(remaining)
    {}
    int faults {0};

protected:
    std::streamsize xsgetn(char* data, std::streamsize size) override
    {
        if (remaining == 0) {
            ++faults;
            throw std::ios_base::failure("Injected included-file read error");
        }
        source.read(data, std::min(size, remaining));
        const auto count = source.gcount();
        remaining -= count;
        return count;
    }

private:
    Base::Reader& source;
    std::streamsize remaining;
};

class ArchiveDiagnostics: public Base::ILogger
{
public:
    ArchiveDiagnostics()
    {
        Base::Console().attachObserver(this);
    }
    ~ArchiveDiagnostics() override
    {
        Base::Console().detachObserver(this);
    }
    void sendLog(
        const std::string&,
        const std::string& message,
        Base::LogStyle style,
        Base::IntendedRecipient,
        Base::ContentType
    ) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        messages.emplace_back(style, message);
    }
    bool contains(Base::LogStyle style, const std::string& text) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return std::any_of(messages.begin(), messages.end(), [&](const auto& entry) {
            return entry.first == style && entry.second.find(text) != std::string::npos;
        });
    }

private:
    mutable std::mutex mutex;
    std::vector<std::pair<Base::LogStyle, std::string>> messages;
};
}  // namespace

// Mode: 0 success; 1 actual output write error; 2 actual fclose error;
// 3 reader streambuf error after one block; 4 create failure; 5 preexisting read-only file.
class IncludedFileArchiveTest: public PropertyFileIncludedTest,
                               public ::testing::WithParamInterface<std::tuple<int, bool>>
{
protected:
    void TearDown() override
    {
        fcIncludedFileFault(nullptr, 0);
        PropertyFileIncludedTest::TearDown();
    }
};

TEST_P(IncludedFileArchiveTest, PreservesDocumentFailurePolicyAndLaterEntry)
{
    const auto [mode, empty] = GetParam();
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const std::string folder = directory.path().toStdString();
    const auto firstData = binaryPayload(empty ? 0 : 131089);
    const auto secondData = binaryPayload(8193);
    writeIncludedBytes(folder + "/first-input", firstData);
    writeIncludedBytes(folder + "/second-input", secondData);
    auto first = static_cast<App::PropertyFileIncluded*>(
        _object->addDynamicProperty("App::PropertyFileIncluded", "AFirst")
    );
    auto second = static_cast<App::PropertyFileIncluded*>(
        _object->addDynamicProperty("App::PropertyFileIncluded", "ZSecond")
    );
    first->setValue((folder + "/first-input").c_str(), "first.bin");
    second->setValue((folder + "/second-input").c_str(), "second.bin");
    const std::string archive = folder + "/two-entries.FCStd";
    ASSERT_TRUE(_doc->saveAs(archive.c_str()));
    App::GetApplication().closeDocument(_doc->getName());
    _doc = App::GetApplication().newDocument("IncludedFileArchiveTarget");

    int before = 0, after = 0, readFaults = 0;
    bool sawSecond = false, failedFirst = false, failedSecond = false;
    std::string firstPath, secondPath;
    fastsignals::scoped_connection beforeConnection = _doc->signalBeforeChangeObject.connect(
        [&](const App::DocumentObject&, const App::Property& property) {
            if (std::string(property.getName()) == "AFirst") {
                ++before;
            }
        }
    );
    fastsignals::scoped_connection afterConnection = _doc->signalChangedObject.connect(
        [&](const App::DocumentObject&, const App::Property& property) {
            if (std::string(property.getName()) == "AFirst") {
                ++after;
            }
        }
    );
    PayloadHook firstHook, secondHook;
    fastsignals::scoped_connection restoreConnection = _doc->signalRestoreDocument.connect(
        [&](Base::XMLReader& xml) {
            _object = _doc->getObject("VarSet");
            first = static_cast<App::PropertyFileIncluded*>(_object->getPropertyByName("AFirst"));
            second = static_cast<App::PropertyFileIncluded*>(_object->getPropertyByName("ZSecond"));
            firstPath = first->getValue();
            secondPath = second->getValue();
            before = after = 0;  // Count payload notifications separately from XML assignment.
            if (mode == 1 || mode == 2) {
                fcIncludedFileFault(firstPath.c_str(), mode);
            }
            if (mode == 4) {
                std::filesystem::create_directory(firstPath);
            }
            if (mode == 5) {
                writeIncludedBytes(firstPath, "preexisting");
                Base::FileInfo(firstPath).setPermissions(Base::FileInfo::ReadOnly);
            }
            firstHook.read = [&](Base::Reader& reader) {
                if (mode == 3) {
                    FailingInput buffer(reader, empty ? 0 : 65536);
                    std::istream input(&buffer);
                    Base::Reader failing(input, reader.getFileName(), reader.getFileVersion());
                    try {
                        first->RestoreDocFile(failing);
                    }
                    catch (...) {
                        readFaults = buffer.faults;
                        throw;
                    }
                }
                else {
                    first->RestoreDocFile(reader);
                }
            };
            secondHook.read = [&, xmlReader = &xml](Base::Reader& reader) {
                // Observe the real XMLReader failure list while the document owns it.
                sawSecond = true;
                failedFirst = xmlReader->hasReadFailed("first.bin");
                second->RestoreDocFile(reader);
                failedSecond = xmlReader->hasReadFailed("second.bin");
            };
            for (auto& entry : xml.FileList) {
                if (entry.Object == first) {
                    entry.Object = &firstHook;
                }
                if (entry.Object == second) {
                    entry.Object = &secondHook;
                }
            }
        }
    );
    ArchiveDiagnostics diagnostics;
    EXPECT_NO_THROW(_doc->restore(archive.c_str()));
    const int ioFaults = fcIncludedFileFaultHits();
    fcIncludedFileFault(nullptr, 0);
    ASSERT_TRUE(sawSecond);
    EXPECT_FALSE(failedSecond);
    EXPECT_EQ(includedBytes(secondPath), secondData);
    EXPECT_FALSE(Base::FileInfo(secondPath).isWritable());
    const bool failed = mode == 2 || mode == 3 || mode == 4 || (mode == 1 && !empty);
    EXPECT_EQ(failedFirst, failed && !empty);
    EXPECT_FALSE(_doc->testStatus(App::Document::RestoreError));
    EXPECT_FALSE(_doc->testStatus(App::Document::PartialRestore));
    EXPECT_FALSE(_doc->testStatus(App::Document::Restoring));
    EXPECT_EQ(before, mode == 4 || mode == 5 ? 0 : 1);
    EXPECT_EQ(after, failed || mode == 5 ? 0 : 1);
    RecordProperty("fault_mode", mode);
    RecordProperty("empty_payload", empty ? 1 : 0);
    RecordProperty("payload_bytes", static_cast<int>(firstData.size()));
    RecordProperty(
        "archive_sha256",
        QCryptographicHash::hash(
            QByteArray::fromStdString(includedBytes(archive)),
            QCryptographicHash::Sha256
        )
            .toHex()
            .toStdString()
    );
    RecordProperty("io_fault_hits", ioFaults);
    RecordProperty("read_fault_hits", readFaults);
    RecordProperty("failed_files_first", failedFirst ? 1 : 0);
    RecordProperty("before_notifications", before);
    RecordProperty("after_notifications", after);
    RecordProperty("later_entry_restored", sawSecond && includedBytes(secondPath) == secondData ? 1 : 0);
    if (mode == 1) {
        EXPECT_EQ(ioFaults > 0, !empty);
    }
    if (mode == 2) {
        EXPECT_GT(ioFaults, 0);
    }
    if (mode == 3) {
        EXPECT_EQ(readFaults, 1);
    }
    if (failed) {
        EXPECT_TRUE(diagnostics.contains(
            empty ? Base::LogStyle::Log : Base::LogStyle::Error,
            empty ? "Skipped empty embedded file:" : "Reading failed from embedded file:"
        ));
    }
    else {
        EXPECT_FALSE(diagnostics.contains(Base::LogStyle::Error, "Reading failed from embedded file:"));
    }
    if (mode == 4) {
        EXPECT_TRUE(std::filesystem::is_directory(firstPath));
    }
    else {
        const auto expected = mode == 5 ? std::string("preexisting")
            : mode == 3                 ? firstData.substr(0, empty ? 0 : 65536)
            : mode == 1 && !empty       ? std::string()
                                        : firstData;
        EXPECT_EQ(includedBytes(firstPath), expected);
        RecordProperty("retained_destination_bytes", static_cast<int>(includedBytes(firstPath).size()));
        RecordProperty("destination_writable", Base::FileInfo(firstPath).isWritable() ? 1 : 0);
        EXPECT_EQ(Base::FileInfo(firstPath).isWritable(), failed);
    }
}

INSTANTIATE_TEST_SUITE_P(
    EmptyAndNonempty,
    IncludedFileArchiveTest,
    ::testing::Combine(::testing::Range(0, 6), ::testing::Bool())
);

TEST_F(PropertyFileIncludedTest, rejectedAssignmentRetainsBytesButUpdatesOriginalName)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const auto input = directory.path().toStdString() + "/source.bin";
    writeIncludedBytes(input, binaryPayload(37));
    _property->setValue(input.c_str(), "assigned.bin");
    const std::string value = _property->getValue();
    const auto missing = directory.path().toStdString() + "/missing.bin";
    int changes = 0, before = 0;
    fastsignals::scoped_connection beforeConnection = _object->signalBeforeChange.connect(
        [&](const App::DocumentObject&, const App::Property& property) {
            if (&property == _property) {
                ++before;
            }
        }
    );
    fastsignals::scoped_connection connection = _object->signalChanged.connect(
        [&](const App::DocumentObject&, const App::Property& property) {
            if (&property == _property) {
                ++changes;
            }
        }
    );
    EXPECT_THROW(_property->setValue(missing.c_str()), Base::FileSystemError);
    EXPECT_EQ(_property->getOriginalFileName(), missing);  // Existing nontransactional metadata.
    EXPECT_EQ(std::string(_property->getValue()), value);
    EXPECT_EQ(includedBytes(value), binaryPayload(37));
    EXPECT_FALSE(Base::FileInfo(value).isWritable());
    EXPECT_EQ(changes, 0);
    EXPECT_EQ(before, 0);
}

class MissingIncludedPayloadTest: public PropertyFileIncludedTest,
                                  public ::testing::WithParamInterface<bool>
{
};

TEST_P(MissingIncludedPayloadTest, SavePreservesPreviousArchive)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const auto folder = directory.path().toStdString();
    writeIncludedBytes(folder + "/input", binaryPayload(GetParam() ? 0 : 37));
    _property->setValue((folder + "/input").c_str(), "payload.bin");
    const auto archive = folder + "/saved.FCStd";
    ASSERT_TRUE(_doc->saveAs(archive.c_str()));
    const auto previous = includedBytes(archive);
    ASSERT_TRUE(Base::FileInfo(_property->getValue()).deleteFile());
    EXPECT_THROW(_doc->save(), Base::FileSystemError);
    EXPECT_EQ(includedBytes(archive), previous);
    EXPECT_FALSE(std::string(_property->getValue()).empty());
    // The failed save's UUID temporary archive remains; the original is not replaced.
    int temporaryArchives = 0;
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.path().filename().string().starts_with("saved.FCStd.")) {
            ++temporaryArchives;
        }
    }
    EXPECT_EQ(temporaryArchives, 1);
}
INSTANTIATE_TEST_SUITE_P(EmptyAndNonempty, MissingIncludedPayloadTest, ::testing::Bool());
#endif
