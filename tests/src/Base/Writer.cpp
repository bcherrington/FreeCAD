// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <iomanip>
#include <locale>
#include <sstream>

#include "Base/Exception.h"
#include "Base/Writer.h"

// Writer is designed to be a base class, so for testing we actually instantiate a StringWriter,
// which is derived from it

class WriterTest: public ::testing::Test
{
protected:
    // void SetUp() override {}

    // void TearDown() override {}
protected:
    Base::StringWriter _writer;
};

TEST_F(WriterTest, insertTextSimple)
{
    // Arrange
    std::string testTextData {"Simple ASCII data"};
    std::string expectedResult {"<![CDATA[" + testTextData + "]]>"};

    // Act
    _writer.insertText(testTextData);

    // Assert
    EXPECT_EQ(expectedResult, _writer.getString());
}

/// If the data happens to actually include an XML CDATA close marker, that needs to be "escaped" --
/// this is done by breaking it up into two separate CDATA sections, splitting apart the marker.
TEST_F(WriterTest, insertTextNeedsEscape)
{
    // Arrange
    std::string testDataA {"ASCII data with a close marker in it, like so: ]]"};
    std::string testDataB {"> "};
    std::string expectedResult {"<![CDATA[" + testDataA + "]]><![CDATA[" + testDataB + "]]>"};

    // Act
    _writer.insertText(testDataA + testDataB);

    // Assert
    EXPECT_EQ(expectedResult, _writer.getString());
}

TEST_F(WriterTest, insertNonAsciiData)
{
    // Arrange
    std::string testData {"\x01\x02\x03\x04\u0001F450😀"};
    std::string expectedResult {"<![CDATA[" + testData + "]]>"};

    // Act
    _writer.insertText(testData);

    // Assert
    EXPECT_EQ(expectedResult, _writer.getString());
}

TEST_F(WriterTest, beginCharStream)
{
    // Arrange & Act
    auto& checkStream {_writer.beginCharStream()};

    // Assert
    EXPECT_TRUE(checkStream.good());
}

TEST_F(WriterTest, beginCharStreamTwice)
{
    // Arrange
    _writer.beginCharStream();

    // Act & Assert
    EXPECT_THROW(_writer.beginCharStream(), Base::RuntimeError);
}

TEST_F(WriterTest, endCharStream)
{
    // Arrange
    _writer.beginCharStream();

    // Act
    _writer.endCharStream();

    // Assert
    EXPECT_EQ("<![CDATA[]]>", _writer.getString());
}

TEST_F(WriterTest, endCharStreamTwice)
{
    // Arrange
    _writer.beginCharStream();
    _writer.endCharStream();

    // Act
    _writer.endCharStream();  // Doesn't throw, or do anything at all

    // Assert
    EXPECT_EQ("<![CDATA[]]>", _writer.getString());
}

TEST_F(WriterTest, charStream)
{
    // Arrange
    auto& streamA {_writer.beginCharStream()};

    // Act
    auto& streamB {_writer.charStream()};

    // Assert
    EXPECT_EQ(&streamA, &streamB);
}

TEST_F(WriterTest, charStreamBase64Encoded)
{
    // Arrange
    _writer.beginCharStream(Base::CharStreamFormat::Base64Encoded);
    std::string data {"FreeCAD rocks! 🪨🪨🪨"};

    // Act
    _writer.charStream() << data;
    _writer.endCharStream();

    // Assert
    // Conversion done using https://www.base64encode.org for testing purposes
    EXPECT_EQ(std::string("RnJlZUNBRCByb2NrcyEg8J+qqPCfqqjwn6qo\n"), _writer.getString());
}


TEST_F(WriterTest, rawTextPreservesEscapingAcrossAllSplitPositions)
{
    const std::string text = std::string(4095, 'a') + "]]><&\"\n😀]]>tail]";
    const std::string expected = "<![CDATA[" + std::string(4095, 'a')
        + "]]]]><![CDATA[><&\"\n😀]]]]><![CDATA[>tail]]]>";
    // Cover caller writes/flushes on both sides of each escape and the filter buffer.
    for (std::size_t split = 4088; split <= text.size(); ++split) {
        SCOPED_TRACE(split);
        Base::StringWriter writer;
        auto& stream = writer.beginCharStream();
        stream.write(text.data(), static_cast<std::streamsize>(split));
        stream.flush();
        stream.write(text.data() + split, static_cast<std::streamsize>(text.size() - split));
        writer.endCharStream();
        EXPECT_EQ(writer.getString(), expected);
    }
}

TEST_F(WriterTest, rawTextPreservesConsecutiveBracketBytes)
{
    // Preserve the established format, including its handling of longer bracket runs.
    const std::string text = "]]]> ]]]]> ]]]]]>";
    const std::string expected = "<![CDATA[]]]> ]]]]> ]]]]]]]><![CDATA[>]]>";
    Base::StringWriter writer;
    auto& stream = writer.beginCharStream();
    for (char ch : text) {
        stream.put(ch);
        stream.flush();
    }
    writer.endCharStream();
    EXPECT_EQ(writer.getString(), expected);
}

TEST_F(WriterTest, rawTextPreservesNumericAndLocaleFormatting)
{
    class Punctuation: public std::numpunct<char>
    {
        char do_decimal_point() const override
        {
            return ',';
        }
        char do_thousands_sep() const override
        {
            return '_';
        }
        std::string do_grouping() const override
        {
            return "\3";
        }
    };
    const std::locale locale(std::locale::classic(), new Punctuation);
    auto emit = [](std::ostream& stream) {
        stream << std::hex << std::showbase << 123456789L << ' ' << std::dec << -123456789L << ' '
               << std::boolalpha << true << ' ' << std::scientific << std::setprecision(16)
               << 1.2345678901234567 << ' ' << std::setw(12) << std::setfill('0') << 42;
    };
    std::ostringstream reference;
    reference.imbue(locale);
    emit(reference);
    auto& stream = _writer.beginCharStream();
    stream.imbue(locale);
    emit(stream);
    _writer.endCharStream();
    EXPECT_EQ(_writer.getString(), "<![CDATA[" + reference.str() + "]]>");
}

TEST_F(WriterTest, rawTextPropagatesSinkException)
{
    class Buffer: public std::streambuf
    {
    public:
        bool fail = false;
        std::streamsize xsputn(const char*, std::streamsize size) override
        {
            if (fail) {
                throw std::runtime_error("injected writer failure");
            }
            return size;
        }
        int_type overflow(int_type ch) override
        {
            if (fail) {
                throw std::runtime_error("injected writer failure");
            }
            return traits_type::not_eof(ch);
        }
    };
    class Writer: public Base::StringWriter
    {
    public:
        Buffer buffer;
        std::ostream output {&buffer};
        std::ostream& Stream() override
        {
            return output;
        }
    } writer;
    auto& stream = writer.beginCharStream();
    stream.exceptions(std::ios::badbit | std::ios::failbit);
    writer.buffer.fail = true;
    EXPECT_ANY_THROW(stream << std::string(16384, 'x') << std::flush);
    EXPECT_TRUE(stream.fail());
    writer.buffer.fail = false;
    stream.exceptions(std::ios::goodbit);
    stream.clear();
    writer.endCharStream();
}
