// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <vector>

#include <QImage>
#include <QColor>

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
