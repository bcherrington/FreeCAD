// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <tuple>

#include <QImage>
#include <QColor>
#include <QPainter>
#include <QSvgRenderer>

#include <Inventor/fields/SoSFImage.h>

#include <src/App/InitApplication.h>

#include <Gui/BitmapFactory.h>

namespace
{

class BitmapFactoryTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

std::vector<unsigned char> imageBytes(const SoSFImage& image, int& width, int& height, int& components)
{
    SbVec2s size;
    const unsigned char* bytes = image.getValue(size, components);
    width = size[0];
    height = size[1];

    if (!bytes) {
        return {};
    }

    return {bytes, bytes + (width * height * components)};
}

void expectCoinImage(
    const QImage& source,
    const std::vector<unsigned char>& expectedBytes,
    int expectedComponents = 4
)
{
    SoSFImage image;
    Gui::BitmapFactory().convert(source, image);

    int width = 0;
    int height = 0;
    int components = 0;
    const std::vector<unsigned char> bytes = imageBytes(image, width, height, components);

    EXPECT_EQ(width, source.width());
    EXPECT_EQ(height, source.height());
    EXPECT_EQ(components, expectedComponents);
    EXPECT_EQ(bytes, expectedBytes);
}

QImage makeArgb32Image()
{
    QImage image(2, 2, QImage::Format_ARGB32);
    image.setPixelColor(0, 0, QColor(10, 20, 30, 40));
    image.setPixelColor(1, 0, QColor(50, 60, 70, 80));
    image.setPixelColor(0, 1, QColor(90, 100, 110, 120));
    image.setPixelColor(1, 1, QColor(130, 140, 150, 160));
    return image;
}

QImage makeRgba8888Image()
{
    QImage image(2, 2, QImage::Format_RGBA8888);

    std::array<unsigned char, 16> values {
        10,
        20,
        30,
        40,
        50,
        60,
        70,
        80,
        90,
        100,
        110,
        120,
        130,
        140,
        150,
        160,
    };

    for (int y = 0; y < image.height(); ++y) {
        std::copy_n(values.data() + (y * image.width() * 4), image.width() * 4, image.scanLine(y));
    }

    return image;
}

}  // namespace

TEST_F(BitmapFactoryTest, ConvertArgb32UsesRgbaComponentsAndFlipsVertically)
{
    const QImage image = makeArgb32Image();

    expectCoinImage(
        image,
        {
            90,
            100,
            110,
            120,
            130,
            140,
            150,
            160,
            10,
            20,
            30,
            40,
            50,
            60,
            70,
            80,
        }
    );
}

TEST_F(BitmapFactoryTest, ConvertRgb32UsesOpaqueAlphaAndFlipsVertically)
{
    QImage image(2, 2, QImage::Format_RGB32);
    image.setPixel(0, 0, qRgb(10, 20, 30));
    image.setPixel(1, 0, qRgb(50, 60, 70));
    image.setPixel(0, 1, qRgb(90, 100, 110));
    image.setPixel(1, 1, qRgb(130, 140, 150));

    expectCoinImage(
        image,
        {
            90,
            100,
            110,
            255,
            130,
            140,
            150,
            255,
            10,
            20,
            30,
            255,
            50,
            60,
            70,
            255,
        }
    );
}

TEST_F(BitmapFactoryTest, ConvertRgba8888UsesByteOrderedRgbaAndFlipsVertically)
{
    const QImage image = makeRgba8888Image();

    expectCoinImage(
        image,
        {
            90,
            100,
            110,
            120,
            130,
            140,
            150,
            160,
            10,
            20,
            30,
            40,
            50,
            60,
            70,
            80,
        }
    );
}

TEST_F(BitmapFactoryTest, ConvertRgbx8888UsesOpaqueAlphaAndFlipsVertically)
{
    QImage image = makeRgba8888Image().convertToFormat(QImage::Format_RGBX8888);

    expectCoinImage(
        image,
        {
            90,
            100,
            110,
            255,
            130,
            140,
            150,
            255,
            10,
            20,
            30,
            255,
            50,
            60,
            70,
            255,
        }
    );
}

TEST_F(BitmapFactoryTest, ConvertNullImageClearsPreviousImage)
{
    SoSFImage image;
    Gui::BitmapFactory().convert(makeArgb32Image(), image);
    Gui::BitmapFactory().convert(QImage(), image);

    int width = -1;
    int height = -1;
    int components = -1;
    EXPECT_TRUE(imageBytes(image, width, height, components).empty());
    EXPECT_EQ(width, 0);
    EXPECT_EQ(height, 0);
    EXPECT_EQ(components, 0);
}

TEST_F(BitmapFactoryTest, ConvertOversizedFallbackExtentClearsPreviousImage)
{
    // 65537 used to narrow to 1: bounded input and allocation, but wrong output size.
    QImage source(65537, 1, QImage::Format_Grayscale8);
    ASSERT_FALSE(source.isNull());
    source.fill(42);
    SoSFImage image;
    Gui::BitmapFactory().convert(makeArgb32Image(), image);
    Gui::BitmapFactory().convert(source, image);

    int width = -1;
    int height = -1;
    int components = -1;
    EXPECT_TRUE(imageBytes(image, width, height, components).empty());
    EXPECT_EQ(width, 0);
    EXPECT_EQ(height, 0);
    EXPECT_EQ(components, 0);
}

class BitmapFactoryExtentTest
    : public BitmapFactoryTest,
      public ::testing::WithParamInterface<std::tuple<int, bool, QImage::Format>>
{
};

TEST_P(BitmapFactoryExtentTest, ConvertsSupportedExtentsAndClearsUnsupportedExtents)
{
    const auto [extent, vertical, format] = GetParam();
    const int width = vertical ? 1 : extent;
    const int height = vertical ? extent : 1;
    QImage source(width, height, format);
    ASSERT_FALSE(source.isNull());
    source.fill(QColor(10, 20, 30));
    source.setPixelColor(0, 0, QColor(40, 50, 60));
    source.setPixelColor(width - 1, height - 1, QColor(70, 80, 90));

    SoSFImage image;
    Gui::BitmapFactory().convert(makeArgb32Image(), image);
    Gui::BitmapFactory().convert(source, image);
    int actualWidth = -1;
    int actualHeight = -1;
    int components = -1;
    const auto bytes = imageBytes(image, actualWidth, actualHeight, components);
    if (extent > 32767) {
        EXPECT_TRUE(bytes.empty());
        EXPECT_EQ(actualWidth, 0);
        EXPECT_EQ(actualHeight, 0);
        EXPECT_EQ(components, 0);
        return;
    }

    ASSERT_EQ(actualWidth, width);
    ASSERT_EQ(actualHeight, height);
    ASSERT_EQ(components, 4);
    ASSERT_EQ(bytes.size(), static_cast<std::size_t>(width) * height * 4);
    std::vector<unsigned char> expected;
    expected.reserve(bytes.size());
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            const QColor pixel = source.pixelColor(x, y);
            expected.push_back(pixel.red());
            expected.push_back(pixel.green());
            expected.push_back(pixel.blue());
            expected.push_back(pixel.alpha());
        }
    }
    EXPECT_EQ(bytes, expected);
}

INSTANTIATE_TEST_SUITE_P(
    CoinSignedShortBoundary,
    BitmapFactoryExtentTest,
    ::testing::Combine(
        ::testing::Values(32767, 32768, 65537),
        ::testing::Bool(),
        ::testing::Values(
            QImage::Format_RGB32,
            QImage::Format_ARGB32,
            QImage::Format_RGBA8888,
            QImage::Format_RGBX8888,
            QImage::Format_Grayscale8,
            QImage::Format_ARGB32_Premultiplied
        )
    )
);

namespace
{
QImage asymmetricImage()
{
    QImage source(5, 3, QImage::Format_ARGB32);
    for (int y = 0; y < source.height(); ++y) {
        for (int x = 0; x < source.width(); ++x) {
            source.setPixelColor(
                x,
                y,
                QColor(
                    20 + 40 * x,
                    30 + 70 * y,
                    15 + 13 * x + 17 * y,
                    std::array<int, 4> {0, 64, 128, 255}[(x + y) % 4]
                )
            );
        }
    }
    return source;
}

std::vector<unsigned char> referenceRgba(const QImage& source)
{
    // Historical pixelColor approach, with explicit RGBA8 output rather than
    // the old sizeInBytes-based component inference (which counted row padding).
    std::vector<unsigned char> result;
    for (int y = source.height() - 1; y >= 0; --y) {
        for (int x = 0; x < source.width(); ++x) {
            const QColor color = source.pixelColor(x, y);
            result.push_back(color.red());
            result.push_back(color.green());
            result.push_back(color.blue());
            result.push_back(color.alpha());
        }
    }
    return result;
}

void expectReference(
    const SoSFImage& image,
    const std::vector<unsigned char>& expected,
    int width,
    int height,
    int maxError = 1
)
{
    int actualWidth, actualHeight, components;
    const auto actual = imageBytes(image, actualWidth, actualHeight, components);
    ASSERT_EQ(actualWidth, width);
    ASSERT_EQ(actualHeight, height);
    ASSERT_EQ(components, 4);
    ASSERT_EQ(actual.size(), expected.size());
    // 16-bit/float and premultiplied quantization may round one RGBA8 level
    // differently in Qt's optimized conversion than QColor's scalar accessor.
    for (std::size_t i = 0; i < actual.size(); ++i) {
        ASSERT_LE(std::abs(int(actual[i]) - int(expected[i])), maxError) << "component " << i;
    }
}
}  // namespace

class BitmapFactoryFormatTest: public BitmapFactoryTest, public ::testing::WithParamInterface<int>
{
};

TEST_P(BitmapFactoryFormatTest, MatchesScalarReferenceAndOwnsPixelsAfterSourceDies)
{
    const auto format = static_cast<QImage::Format>(GetParam());
    const int maxError = (format == QImage::Format_RGB32 || format == QImage::Format_ARGB32
                          || format == QImage::Format_RGBX8888 || format == QImage::Format_RGBA8888)
        ? 0
        : 1;
    SoSFImage image;
    std::vector<unsigned char> expected;
    {
        QImage source = asymmetricImage().convertToFormat(format);
        ASSERT_FALSE(source.isNull());
        expected = referenceRgba(source);
        Gui::BitmapFactory().convert(source, image);
        expectReference(image, expected, 5, 3, maxError);
        std::fill_n(source.bits(), source.sizeInBytes(), 0xA5);
    }
    const std::vector<unsigned char> overwriteFreedStorage(65536, 0xA5);
    expectReference(image, expected, 5, 3, maxError);
    SoSFImage copy;
    copy = image;
    image.setValue(SbVec2s(0, 0), 0, nullptr);
    expectReference(copy, expected, 5, 3, maxError);
}

INSTANTIATE_TEST_SUITE_P(
    AllQtFormats,
    BitmapFactoryFormatTest,
    ::testing::Range(int(QImage::Format_Mono), int(QImage::NImageFormats))
);

TEST_F(BitmapFactoryTest, IndexedColorAndGrayAlphaPalettesPreserveTransparency)
{
    for (bool gray : {false, true}) {
        QImage source(5, 3, QImage::Format_Indexed8);
        source.setColorTable(
            gray ? QList<QRgb> {qRgba(10, 10, 10, 255), qRgba(80, 80, 80, 128), qRgba(160, 160, 160, 0)}
                 : QList<QRgb> {qRgba(255, 0, 0, 255), qRgba(0, 255, 0, 128), qRgba(0, 0, 255, 0)}
        );
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 5; ++x) {
                source.setPixel(x, y, (x + y) % 3);
            }
        }
        SoSFImage image;
        Gui::BitmapFactory().convert(source, image);
        expectReference(image, referenceRgba(source), 5, 3);
    }
}

TEST_F(BitmapFactoryTest, HighDepthNonByteAlignedChannelsQuantizeToRgba8)
{
    QImage source(5, 3, QImage::Format_RGBA64);
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 5; ++x) {
            source.setPixelColor(
                x,
                y,
                QColor::fromRgba64(
                    1001 + x * 11000,
                    3003 + y * 21000,
                    65501 - x * 10000,
                    5001 + (x + y) * 8000
                )
            );
        }
    }
    SoSFImage image;
    Gui::BitmapFactory().convert(source, image);
    expectReference(image, referenceRgba(source), 5, 3);
}

TEST_F(BitmapFactoryTest, PaddedExternalScanlinesAreCopiedBeforeCallerStorageDies)
{
    SoSFImage image;
    std::vector<unsigned char> expected;
    {
        std::vector<unsigned char> external(32 * 3, 0xCD);
        QImage source(external.data(), 5, 3, 32, QImage::Format_ARGB32);
        const QImage pattern = asymmetricImage();
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 5; ++x) {
                source.setPixelColor(x, y, pattern.pixelColor(x, y));
            }
        }
        expected = referenceRgba(source);
        Gui::BitmapFactory().convert(source, image);
        std::fill(external.begin(), external.end(), 0);
    }
    expectReference(image, expected, 5, 3);
}

TEST_F(BitmapFactoryTest, SvgDerivedPixelsPreserveColorAlphaAndOrientation)
{
    const QByteArray svg
        = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"20\" height=\"12\">"
          "<rect width=\"10\" height=\"6\" fill=\"red\"/>"
          "<rect x=\"10\" width=\"10\" height=\"6\" fill=\"green\" fill-opacity=\"0.5\"/>"
          "<rect y=\"6\" width=\"10\" height=\"6\" fill=\"blue\"/>"
          "<rect x=\"10\" y=\"6\" width=\"10\" height=\"6\" fill=\"yellow\"/></svg>";
    QSvgRenderer renderer(svg);
    ASSERT_TRUE(renderer.isValid());
    QImage source(20, 12, QImage::Format_ARGB32_Premultiplied);
    source.fill(Qt::transparent);
    {
        QPainter painter(&source);
        renderer.render(&painter);
    }
    SoSFImage image;
    Gui::BitmapFactory().convert(source, image);
    expectReference(image, referenceRgba(source), 20, 12);
}
