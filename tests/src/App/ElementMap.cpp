// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <App/Application.h>
#include <App/ElementMap.h>
#include <App/ElementMapRestore.h>
#include <Base/Exception.h>
#include <src/App/InitApplication.h>

#include <iomanip>
#include <climits>
#include <cstdlib>
#include <limits>
#include <vector>
#include <sstream>

// NOLINTBEGIN(readability-magic-numbers)


// this is a "holder" class used for simpler testing of ElementMap in the context of a class
class LessComplexPart
{
public:
    LessComplexPart(long tag, const std::string& nameStr, App::StringHasherRef hasher)
        : elementMapPtr(std::make_shared<Data::ElementMap>())
        , Tag(tag)
        , name(nameStr)
    {
        // object also have Vertexes etc and the face count varies; but that is not important
        // here since we are not testing a real model
        // the "MappedName" is left blank for now
        Data::IndexedName face1("Face", 1);
        Data::IndexedName face2("Face", 2);
        Data::IndexedName face3("Face", 3);
        Data::IndexedName face4("Face", 4);
        Data::IndexedName face5("Face", 5);
        Data::IndexedName face6("Face", 6);
        elementMapPtr->hasher = hasher;
        elementMapPtr->setElementName(face1, Data::MappedName(face1), Tag);
        elementMapPtr->setElementName(face2, Data::MappedName(face2), Tag);
        elementMapPtr->setElementName(face3, Data::MappedName(face3), Tag);
        elementMapPtr->setElementName(face4, Data::MappedName(face4), Tag);
        elementMapPtr->setElementName(face5, Data::MappedName(face5), Tag);
        elementMapPtr->setElementName(face6, Data::MappedName(face6), Tag);
    }

    Data::ElementMapPtr elementMapPtr;
    mutable long Tag;
    Data::MappedName name;
};

class ElementMapTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        App::GetApplication().newDocument(_docName.c_str(), "testUser");
        _sids = &_sid;
        _hasher = Base::Reference<App::StringHasher>(new App::StringHasher);
        ASSERT_EQ(_hasher.getRefCount(), 1);
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    std::string _docName;
    Data::ElementIDRefs _sid;
    QVector<App::StringIDRef>* _sids;
    App::StringHasherRef _hasher;
};

TEST_F(ElementMapTest, defaultConstruction)
{
    // Act
    Data::ElementMap elementMap = Data::ElementMap();

    // Assert
    EXPECT_EQ(elementMap.size(), 0);
}

TEST_F(ElementMapTest, setElementNameDefaults)
{
    // Arrange
    Data::ElementMap elementMap;
    Data::IndexedName element("Edge", 1);
    Data::MappedName mappedName("TEST");

    // Act
    auto resultName = elementMap.setElementName(element, mappedName, 0);
    auto mappedToElement = elementMap.find(element);

    // Assert
    EXPECT_EQ(resultName, mappedName);
    EXPECT_EQ(mappedToElement, mappedName);
}

TEST_F(ElementMapTest, setElementNameNoOverwrite)
{
    // Arrange
    Data::ElementMap elementMap;
    Data::IndexedName element("Edge", 1);
    Data::MappedName mappedName("TEST");
    Data::MappedName anotherMappedName("ANOTHERTEST");

    // Act
    auto resultName = elementMap.setElementName(element, mappedName, 0);
    auto resultName2 = elementMap.setElementName(element, anotherMappedName, 0, _sids, false);
    auto mappedToElement = elementMap.find(element);
    auto findAllResult = elementMap.findAll(element);

    // Assert
    EXPECT_EQ(resultName, mappedName);
    EXPECT_EQ(resultName2, anotherMappedName);
    EXPECT_EQ(mappedToElement, mappedName);
    EXPECT_EQ(findAllResult.size(), 2);
    EXPECT_EQ(findAllResult[0].first, mappedName);
    EXPECT_EQ(findAllResult[1].first, anotherMappedName);
}

TEST_F(ElementMapTest, setElementNameWithOverwrite)
{
    // Arrange
    Data::ElementMap elementMap;
    Data::IndexedName element("Edge", 1);
    Data::MappedName mappedName("TEST");
    Data::MappedName anotherMappedName("ANOTHERTEST");

    // Act
    auto resultName = elementMap.setElementName(element, mappedName, 0);
    auto resultName2 = elementMap.setElementName(element, anotherMappedName, 0, _sids, true);
    auto mappedToElement = elementMap.find(element);
    auto findAllResult = elementMap.findAll(element);

    // Assert
    EXPECT_EQ(resultName, mappedName);
    EXPECT_EQ(resultName2, anotherMappedName);
    EXPECT_EQ(mappedToElement, anotherMappedName);
    EXPECT_EQ(findAllResult.size(), 1);
    EXPECT_EQ(findAllResult[0].first, anotherMappedName);
}

TEST_F(ElementMapTest, setElementNameWithHashing)
{
    // Arrange
    Data::ElementMap elementMap;
    std::ostringstream ss;
    Data::IndexedName element("Edge", 1);
    Data::MappedName elementNameHolder(element);  // Will get modified by the encoder
    const Data::MappedName expectedName(element);

    // Act
    elementMap.encodeElementName(element.getType()[0], elementNameHolder, ss, nullptr, 0, nullptr, 0);
    auto resultName = elementMap.setElementName(element, elementNameHolder, 0, _sids);
    auto mappedToElement = elementMap.find(element);

    // Assert
    EXPECT_EQ(resultName, expectedName);
    EXPECT_EQ(mappedToElement, expectedName);
}

TEST_F(ElementMapTest, eraseMappedName)
{
    // Arrange
    Data::ElementMap elementMap;
    Data::IndexedName element("Edge", 1);
    Data::MappedName mappedName("TEST");
    Data::MappedName anotherMappedName("ANOTHERTEST");
    elementMap.setElementName(element, mappedName, 0);
    elementMap.setElementName(element, anotherMappedName, 0);

    // Act
    auto sizeBefore = elementMap.size();
    auto findAllBefore = elementMap.findAll(element);

    elementMap.erase(anotherMappedName);
    auto sizeAfter = elementMap.size();
    auto findAllAfter = elementMap.findAll(element);

    elementMap.erase(anotherMappedName);
    auto sizeAfterRepeat = elementMap.size();
    auto findAllAfterRepeat = elementMap.findAll(element);

    // Assert
    EXPECT_EQ(sizeBefore, 2);
    EXPECT_EQ(findAllBefore.size(), 2);
    EXPECT_EQ(findAllBefore[0].first, mappedName);
    EXPECT_EQ(findAllBefore[1].first, anotherMappedName);

    EXPECT_EQ(sizeAfter, 1);
    EXPECT_EQ(findAllAfter.size(), 1);
    EXPECT_EQ(findAllAfter[0].first, mappedName);

    EXPECT_EQ(sizeAfterRepeat, 1);
    EXPECT_EQ(findAllAfterRepeat.size(), 1);
    EXPECT_EQ(findAllAfterRepeat[0].first, mappedName);
}

TEST_F(ElementMapTest, eraseIndexedName)
{
    // Arrange
    // Create two elements, edge1 and edge2, that have two mapped names each.
    Data::ElementMap elementMap;

    Data::IndexedName element("Edge", 1);
    Data::MappedName mappedName("TEST");
    Data::MappedName anotherMappedName("ANOTHERTEST");
    elementMap.setElementName(element, mappedName, 0);
    elementMap.setElementName(element, anotherMappedName, 0);

    Data::IndexedName element2("Edge", 2);
    Data::MappedName mappedName2("TEST2");
    Data::MappedName anotherMappedName2("ANOTHERTEST2");
    elementMap.setElementName(element2, mappedName2, 0);
    elementMap.setElementName(element2, anotherMappedName2, 0);

    // Act
    auto sizeBefore = elementMap.size();
    auto findAllBefore = elementMap.findAll(element2);

    elementMap.erase(element2);
    auto sizeAfter = elementMap.size();
    auto findAllAfter = elementMap.findAll(element2);

    elementMap.erase(element2);
    auto sizeAfterRepeat = elementMap.size();
    auto findAllAfterRepeat = elementMap.findAll(element2);

    // Assert
    EXPECT_EQ(sizeBefore, 4);
    EXPECT_EQ(findAllBefore.size(), 2);
    EXPECT_EQ(findAllBefore[0].first, mappedName2);
    EXPECT_EQ(findAllBefore[1].first, anotherMappedName2);

    EXPECT_EQ(sizeAfter, 2);
    EXPECT_EQ(findAllAfter.size(), 0);

    EXPECT_EQ(sizeAfterRepeat, 2);
    EXPECT_EQ(findAllAfterRepeat.size(), 0);
}

TEST_F(ElementMapTest, findMappedName)
{
    // Arrange
    // Create two elements, edge1 and edge2, that have two mapped names each.
    Data::ElementMap elementMap;

    Data::IndexedName element("Edge", 1);
    Data::MappedName mappedName("TEST");
    Data::MappedName anotherMappedName("ANOTHERTEST");
    elementMap.setElementName(element, mappedName, 0);
    elementMap.setElementName(element, anotherMappedName, 0);

    Data::IndexedName element2("Edge", 2);
    Data::MappedName mappedName2("TEST2");
    Data::MappedName anotherMappedName2("ANOTHERTEST2");
    elementMap.setElementName(element2, mappedName2, 0);
    elementMap.setElementName(element2, anotherMappedName2, 0);

    // Act
    auto findResult = elementMap.find(mappedName);
    auto findResult2 = elementMap.find(mappedName2);

    // Assert
    EXPECT_EQ(findResult, element);
    EXPECT_EQ(findResult2, element2);
}

TEST_F(ElementMapTest, findIndexedName)
{
    // Arrange
    // Create two elements, edge1 and edge2, that have two mapped names each.
    Data::ElementMap elementMap;

    Data::IndexedName element("Edge", 1);
    Data::MappedName mappedName("TEST");
    Data::MappedName anotherMappedName("ANOTHERTEST");
    elementMap.setElementName(element, mappedName, 0);
    elementMap.setElementName(element, anotherMappedName, 0);

    Data::IndexedName element2("Edge", 2);
    Data::MappedName mappedName2("TEST2");
    Data::MappedName anotherMappedName2("ANOTHERTEST2");
    elementMap.setElementName(element2, mappedName2, 0);
    elementMap.setElementName(element2, anotherMappedName2, 0);

    // Act
    // they return the first mapped name
    auto findResult = elementMap.find(element);
    auto findResult2 = elementMap.find(element2);

    // Assert
    EXPECT_EQ(findResult, mappedName);
    EXPECT_EQ(findResult2, mappedName2);
}

TEST_F(ElementMapTest, findAll)
{
    // Arrange
    // Create two elements, edge1 and edge2, that have two mapped names each.
    Data::ElementMap elementMap;

    Data::IndexedName element("Edge", 1);
    Data::MappedName mappedName("TEST");
    Data::MappedName anotherMappedName("ANOTHERTEST");
    elementMap.setElementName(element, mappedName, 0);
    elementMap.setElementName(element, anotherMappedName, 0);

    Data::IndexedName element2("Edge", 2);
    Data::MappedName mappedName2("TEST2");
    Data::MappedName anotherMappedName2("ANOTHERTEST2");
    elementMap.setElementName(element2, mappedName2, 0);
    elementMap.setElementName(element2, anotherMappedName2, 0);

    // Act
    // they return the first mapped name
    auto findResult = elementMap.findAll(element);
    auto findResult2 = elementMap.findAll(element2);

    // Assert
    EXPECT_EQ(findResult.size(), 2);
    EXPECT_EQ(findResult[0].first, mappedName);
    EXPECT_EQ(findResult[1].first, anotherMappedName);
    EXPECT_EQ(findResult2.size(), 2);
    EXPECT_EQ(findResult2[0].first, mappedName2);
    EXPECT_EQ(findResult2[1].first, anotherMappedName2);
}

TEST_F(ElementMapTest, mimicOnePart)
{
    // Arrange
    //   pattern: new doc, create Cube
    //   for a single part, there is no "naming algo" to speak of
    std::ostringstream ss;
    auto docName = "Unnamed";
    LessComplexPart cube(1L, "Box", _hasher);

    // Act
    auto children = cube.elementMapPtr->getAll();
    ss << docName << "#" << cube.name << "."
       << cube.elementMapPtr->find(Data::IndexedName("Face", 6));

    // Assert
    EXPECT_EQ(children.size(), 6);
    EXPECT_EQ(children[0].index.toString(), "Face1");
    EXPECT_EQ(children[0].name.toString(), "Face1");
    EXPECT_EQ(children[1].index.toString(), "Face2");
    EXPECT_EQ(children[1].name.toString(), "Face2");
    EXPECT_EQ(children[2].index.toString(), "Face3");
    EXPECT_EQ(children[2].name.toString(), "Face3");
    EXPECT_EQ(children[3].index.toString(), "Face4");
    EXPECT_EQ(children[3].name.toString(), "Face4");
    EXPECT_EQ(children[4].index.toString(), "Face5");
    EXPECT_EQ(children[4].name.toString(), "Face5");
    EXPECT_EQ(children[5].index.toString(), "Face6");
    EXPECT_EQ(children[5].name.toString(), "Face6");
    EXPECT_EQ(ss.str(), "Unnamed#Box.Face6");
}

TEST_F(ElementMapTest, mimicSimpleUnion)
{
    // Arrange
    //   pattern: new doc, create Cube, create Cylinder, Union of both (Cube first)
    std::ostringstream ss;
    std::ostringstream finalSs;
    const char* docName = "Unnamed";

    LessComplexPart cube(1L, "Box", _hasher);
    LessComplexPart cylinder(2L, "Cylinder", _hasher);
    // Union (Fusion) operation via the Part Workbench
    LessComplexPart unionPart(3L, "Fusion", _hasher);

    // we are only going to simulate one face for testing purpose
    Data::IndexedName uface3("Face", 3);
    auto PartOp = "FUS";  // Part::OpCodes::Fuse;

    // Act
    //   act: simulate a union/fuse operation
    auto parent = cube.elementMapPtr->getAll()[5];
    Data::MappedName postfixHolder(std::string(Data::POSTFIX_MOD) + "2");
    unionPart.elementMapPtr->encodeElementName(
        postfixHolder[0],
        postfixHolder,
        ss,
        nullptr,
        unionPart.Tag,
        nullptr,
        unionPart.Tag
    );
    auto postfixStr = postfixHolder.toString() + Data::ELEMENT_MAP_PREFIX + PartOp;

    //   act: with the fuse op, name against the cube's Face6
    Data::MappedName uface3Holder(parent.index);
    // we will invoke the encoder for face 3
    unionPart.elementMapPtr->encodeElementName(
        uface3Holder[0],
        uface3Holder,
        ss,
        nullptr,
        unionPart.Tag,
        postfixStr.c_str(),
        cube.Tag
    );
    unionPart.elementMapPtr->setElementName(uface3, uface3Holder, unionPart.Tag, nullptr, true);

    // act: generate a full toponame string for testing  purposes
    finalSs << docName << "#" << unionPart.name;
    finalSs << ".";
    finalSs << Data::ELEMENT_MAP_PREFIX + unionPart.elementMapPtr->find(uface3).toString();
    finalSs << ".";
    finalSs << uface3;

    // Assert
    EXPECT_EQ(postfixStr, ":M2;FUS");
    EXPECT_EQ(unionPart.elementMapPtr->find(uface3).toString(), "Face6;:M2;FUS;:H1:8,F");
    EXPECT_EQ(finalSs.str(), "Unnamed#Fusion.;Face6;:M2;FUS;:H1:8,F.Face3");

    // explanation of "Fusion.;Face6;:M2;FUS;:H2:3,F.Face3" toponame
    // Note: every postfix is prefixed by semicolon
    // Note: the start/middle/end are separated by periods
    //
    // "Fusion" means that we are on the "Fusion" object.
    // "." we are done with the first part
    // ";Face6" means default inheritance comes from face 6 of the parent (which is a cube)
    // ";:M2" means that a Workbench op has happened. "M" is the "Mod" directory in the source tree?
    // ";FUS" means that a Fusion operation has happened. Notice the lack of a colon.
    // ";:H2" means the subtending object (cylinder) has a tag of 2
    // ":3" means the writing position is 3; literally how far into the current postfix we are
    // ",F" means are of type "F" which is short for "Face" of Face3 of Fusion.
    // "." we are done with the second part
    // "Face3" is the localized name
}

TEST_F(ElementMapTest, mimicOperationAgainstSelf)
{
    // Arrange
    //   pattern: new doc, create Cube, Mystery Op with self as target
    std::ostringstream ss;
    LessComplexPart finalPart(99L, "MysteryOp", _hasher);
    // we are only going to simulate one face for testing purpose
    Data::IndexedName uface3("Face", 3);
    auto PartOp = "MYS";
    auto ownFace6 = finalPart.elementMapPtr->getAll()[5];
    Data::MappedName uface3Holder(ownFace6.index);
    auto workbenchId = std::string(Data::POSTFIX_MOD) + "9999";

    // Act
    //   act: with the mystery op, name against its own Face6 for some reason
    Data::MappedName postfixHolder(workbenchId);
    finalPart.elementMapPtr->encodeElementName(
        postfixHolder[0],
        postfixHolder,
        ss,
        nullptr,
        finalPart.Tag,
        nullptr,
        finalPart.Tag
    );
    auto postfixStr = postfixHolder.toString() + Data::ELEMENT_MAP_PREFIX + PartOp;
    // we will invoke the encoder for face 3
    finalPart.elementMapPtr->encodeElementName(
        uface3Holder[0],
        uface3Holder,
        ss,
        nullptr,
        finalPart.Tag,
        postfixStr.c_str(),
        finalPart.Tag
    );
    // override not forced
    finalPart.elementMapPtr->setElementName(uface3, uface3Holder, finalPart.Tag, nullptr, false);

    // Assert
    EXPECT_EQ(postfixStr, ":M9999;MYS");
    EXPECT_EQ(finalPart.elementMapPtr->find(uface3).toString(), "Face3");  // override not forced
    EXPECT_EQ(uface3Holder.toString(), "Face6;:M9999;MYS;:H63:b,F");
    // explaining ";Face6;:M2;MYS;:H2:3,F" name:
    //
    // ";Face6" means default inheritance comes from face 6 of the ownFace6 (which is itself)
    // ";:M9999" means that a Workbench op happened. "M" is the "Mod" directory in the source tree?
    // ";MYS" means that a "Mystery" operation has happened. Notice the lack of a colon.
    // ";:H63" means the subtending object (cylinder) has a tag of 99 (63 in hex)
    // ":b" means the writing position is b (hex); literally how far into the current postfix we are
    // ",F" means are of type "F" which is short for "Face" of Face3 of Fusion.
}

TEST_F(ElementMapTest, hasChildElementMapTest)
{
    // Arrange
    Data::ElementMap::MappedChildElements child
        = {Data::IndexedName("face", 1), 2, 7, 4L, Data::ElementMapPtr(), QByteArray(""), _sid};
    std::vector<Data::ElementMap::MappedChildElements> children = {child};
    LessComplexPart cubeFull(3L, "FullBox", _hasher);
    cubeFull.elementMapPtr->addChildElements(cubeFull.Tag, children);
    //
    LessComplexPart cubeWithoutChildren(2L, "EmptyBox", _hasher);

    // Act
    bool resultFull = cubeFull.elementMapPtr->hasChildElementMap();
    bool resultWhenEmpty = cubeWithoutChildren.elementMapPtr->hasChildElementMap();

    // Assert
    EXPECT_TRUE(resultFull);
    EXPECT_FALSE(resultWhenEmpty);
}

TEST_F(ElementMapTest, hashChildMapsTest)
{
    // Arrange
    LessComplexPart cube(1L, "Box", _hasher);
    auto childOneName = Data::IndexedName("Ping", 1);
    Data::ElementMap::MappedChildElements childOne = {
        childOneName,
        2,
        7,
        3L,
        Data::ElementMapPtr(),
        QByteArray("abcdefghij"),  // postfix must be 10 or more bytes to invoke hasher
        _sid
    };
    std::vector<Data::ElementMap::MappedChildElements> children = {childOne};
    cube.elementMapPtr->addChildElements(cube.Tag, children);
    auto before = _hasher->getIDMap();

    // Act
    cube.elementMapPtr->hashChildMaps(cube.Tag);

    // Assert
    auto after = _hasher->getIDMap();
    EXPECT_EQ(before.size(), 0);
    EXPECT_EQ(after.size(), 1);
}

TEST_F(ElementMapTest, addAndGetChildElementsTest)
{
    // Arrange
    LessComplexPart cube(1L, "Box", _hasher);
    Data::ElementMap::MappedChildElements childOne = {
        Data::IndexedName("Ping", 1),
        2,
        7,
        3L,
        Data::ElementMapPtr(),
        QByteArray("abcdefghij"),  // postfix must be 10 or more bytes to invoke hasher
        _sid
    };
    Data::ElementMap::MappedChildElements childTwo
        = {Data::IndexedName("Pong", 2), 2, 7, 4L, Data::ElementMapPtr(), QByteArray("abc"), _sid};
    std::vector<Data::ElementMap::MappedChildElements> children = {childOne, childTwo};

    // Act
    cube.elementMapPtr->addChildElements(cube.Tag, children);
    auto result = cube.elementMapPtr->getChildElements();

    // Assert
    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(std::any_of(result.begin(), result.end(), [](Data::ElementMap::MappedChildElements e) {
        return e.indexedName.toString() == "Ping1";
    }));
    EXPECT_TRUE(std::any_of(result.begin(), result.end(), [](Data::ElementMap::MappedChildElements e) {
        return e.indexedName.toString() == "Pong2";
    }));
}

TEST_F(ElementMapTest, restoreParsesDotSeparatedEntriesWithoutBoostSplit)
{
    // Arrange
    auto nameSid = _hasher->getID("restore-name-sid");
    auto childSid = _hasher->getID("restore-child-sid");

    std::stringstream stream;
    stream << "987654 PostfixCount 2\n"
           << "Face\n"
           << ";Postfix\n"
           << "\nMapCount 1\n"
           << "\nElementMap 1 987654 1\n"
           << "\nFace\n"
           << "\nChildCount 1\n"
           << "3 0 2 7 0 ChildPostfix 0." << childSid.value() << "\n"
           << "\nNameCount 2\n"
           << "0\n"
           << std::hex << ";CustomMapped.2." << nameSid.value() << " :1.2.0." << nameSid.value()
           << " 0\n"
           << "\nEndMap\n";

    auto elementMap = std::make_shared<Data::ElementMap>();

    // Act
    auto restored = elementMap->restore(_hasher, stream);
    Data::ElementIDRefs restoredNameSids;
    auto mappedName = restored->find(Data::IndexedName("Face", 1), &restoredNameSids);
    auto mappedIndex = restored->find(Data::MappedName("CustomMapped;Postfix"));
    auto childElements = restored->getChildElements();

    // Assert
    EXPECT_EQ(mappedName.toString(), "CustomMapped;Postfix");
    EXPECT_EQ(mappedIndex.toString(), "Face1");
    ASSERT_EQ(restoredNameSids.size(), 1);
    EXPECT_EQ(restoredNameSids.front().value(), nameSid.value());
    ASSERT_EQ(childElements.size(), 1);
    EXPECT_EQ(childElements.front().indexedName.toString(), "Face3");
    ASSERT_EQ(childElements.front().sids.size(), 1);
    EXPECT_EQ(childElements.front().sids.front().value(), childSid.value());
}
namespace
{
std::string formatRestoreLong(long value, int base)
{
    // Stream formatting negative hex values emits unsigned representations.
    const auto magnitude = value < 0 ? 0UL - static_cast<unsigned long>(value)
                                     : static_cast<unsigned long>(value);
    std::ostringstream out;
    if (value < 0) {
        out << '-';
    }
    out << std::setbase(base) << magnitude;
    return out.str();
}

std::vector<std::string> numericRestoreTokens(int base)
{
    const auto maximum = formatRestoreLong(LONG_MAX, base);
    const auto minimum = formatRestoreLong(LONG_MIN, base);
    std::vector<std::string> tokens {
        "",
        "0",
        "00",
        "+0",
        "-0",
        "1",
        "-1",
        "+1",
        " 1",
        "\t-1",
        "\n+1",
        "\v1",
        "\f1",
        "\r1",
        "1suffix",
        "1 2",
        "1+2",
        "1-2",
        "bad",
        "+",
        "-",
        "--1",
        "+-1",
        "0x",
        "-0x",
        "0x1",
        "+0Xf",
        "-0xf",
        "0xg",
        "012",
        "08",
        "a",
        "A",
        "fF",
        "fftail",
        "g1",
        "0b11",
        maximum,
        "+" + maximum,
        minimum,
        maximum + "0",
        minimum + "0",
        std::string(4096, '9'),
        "-" + std::string(4096, '9'),
        std::string(4096, '0') + "1",
        maximum + "suffix",
        minimum + "suffix",
        std::string("12\0ffff", 7),
        std::string(
            "\xff"
            "1",
            2
        ),
        formatRestoreLong(INT_MAX, base),
        formatRestoreLong(INT_MIN, base)
    };
    if (base == 16) {
        tokens.push_back("0x" + maximum);
        tokens.push_back("-0X" + minimum.substr(1));
        tokens.push_back("0x" + std::string(4096, 'f'));
    }
    else {
        tokens.push_back("9223372036854775808");
        tokens.push_back("-9223372036854775809");
        tokens.push_back("2147483648");
        tokens.push_back("-2147483649");
    }
    return tokens;
}
}  // namespace

class ElementMapRestoreNumberTest: public ::testing::TestWithParam<int>
{
};

TEST_P(ElementMapRestoreNumberTest, MatchesLegacyStrtolValues)
{
    const int base = GetParam();
    for (const auto& token : numericRestoreTokens(base)) {
        SCOPED_TRACE(
            ::testing::Message() << "base=" << base << " size=" << token.size()
                                 << " prefix=" << token.substr(0, 80)
        );
        const long expected = std::strtol(token.c_str(), nullptr, base);
        EXPECT_EQ(Data::detail::parseRestoreLong(token, base), expected);
    }
}

TEST_P(ElementMapRestoreNumberTest, BoundedViewsNeedNoNullTerminator)
{
    const int base = GetParam();
    const char bytes[] {' ', '-', '1', '2', '3'};
    EXPECT_EQ(Data::detail::parseRestoreLong(std::string_view(bytes, 3), base), -1);
    EXPECT_EQ(Data::detail::parseRestoreLong(std::string_view(bytes, 0), base), 0);
    auto storage = std::make_unique<char[]>(3);
    storage[0] = '+';
    storage[1] = '1';
    storage[2] = '2';
    EXPECT_EQ(
        Data::detail::parseRestoreLong(std::string_view(storage.get(), 3), base),
        base == 16 ? 18 : 12
    );
}

TEST_P(ElementMapRestoreNumberTest, DifferentialTokenCorpus)
{
    const int base = GetParam();
    // Deterministic token construction; the oracle is the legacy C-library conversion.
    const std::string alphabet = "0123456789abcdefABCDEFxX+- \t_z";
    unsigned long state = 0x5eedUL;
    for (int i = 0; i < 2048; ++i) {
        std::string token;
        const int length = i % 48;
        for (int j = 0; j < length; ++j) {
            state = state * 1664525UL + 1013904223UL;
            token.push_back(alphabet[(state >> 8) % alphabet.size()]);
        }
        SCOPED_TRACE(::testing::Message() << "base=" << base << " row=" << i << " token=" << token);
        EXPECT_EQ(
            Data::detail::parseRestoreLong(token, base),
            std::strtol(token.c_str(), nullptr, base)
        );
    }
}

INSTANTIATE_TEST_SUITE_P(DecimalAndHex, ElementMapRestoreNumberTest, ::testing::Values(10, 16));

TEST_F(ElementMapTest, restoreNumericPrefixCompatibility)
{
    auto sid = _hasher->getID("numeric-sid");
    const std::string sidDecimal = std::to_string(sid.value());
    const auto sidHex = formatRestoreLong(sid.value(), 16);
    std::stringstream stream;
    stream << "987655 PostfixCount 1\nFace\nMapCount 1\n"
           << "ElementMap 1 987655 1\nFace\nChildCount 1\n"
           << "3 0 2 7 0 ChildPostfix 0.+" << sidDecimal << "suffix\n"
           << "NameCount 2\n0\n:0X1.+0x2.0.+0x" << sidHex << "tail 0\nEndMap\n";
    auto map = std::make_shared<Data::ElementMap>()->restore(_hasher, stream);
    Data::ElementIDRefs sids;
    EXPECT_EQ(map->find(Data::IndexedName("Face", 1), &sids).toString(), "Face2");
    ASSERT_EQ(sids.size(), 1);
    EXPECT_EQ(sids.front().value(), sid.value());
    const auto children = map->getChildElements();
    ASSERT_EQ(children.size(), 1);
    ASSERT_EQ(children.front().sids.size(), 1);
    EXPECT_EQ(children.front().sids.front().value(), sid.value());
}

TEST_F(ElementMapTest, restoreRejectsOverflowedNameIndex)
{
    std::stringstream stream;
    stream << "987656 PostfixCount 1\nFace\nMapCount 1\n"
           << "ElementMap 1 987656 1\nFace\nChildCount 0\nNameCount 1\n:"
           << formatRestoreLong(LONG_MAX, 16) << "f.1.0 0\nEndMap\n";
    EXPECT_THROW(std::make_shared<Data::ElementMap>()->restore(_hasher, stream), Base::RuntimeError);
}
TEST_F(ElementMapTest, restoreRejectsUnrepresentableMappedIndices)
{
    std::ostringstream outsideInt;
    outsideInt << std::hex << static_cast<unsigned long>(INT_MAX) + 1UL;
    std::vector<std::string> tokens {
        "-1",
        outsideInt.str(),
        formatRestoreLong(LONG_MIN, 16),
        std::string(4096, 'f')
    };
    unsigned id = 988000;
    for (const auto& token : tokens) {
        SCOPED_TRACE(token.substr(0, 80));
        ++id;
        std::stringstream input;
        input << id << " PostfixCount 1\nFace\nMapCount 1\nElementMap 1 " << id
              << " 1\nFace\nChildCount 0\nNameCount 2\n0\n:1." << token << ".0 0\nEndMap\n";
        EXPECT_THROW(std::make_shared<Data::ElementMap>()->restore(_hasher, input), Base::RuntimeError);
    }
}

TEST_F(ElementMapTest, restoreAcceptsRepresentableMappedIndexLimits)
{
    unsigned id = 988100;
    for (int value : {0, INT_MAX}) {
        ++id;
        std::stringstream input;
        input << id << " PostfixCount 1\nFace\nMapCount 1\nElementMap 1 " << id
              << " 1\nFace\nChildCount 0\nNameCount 2\n0\n:1." << formatRestoreLong(value, 16)
              << ".0 0\nEndMap\n";
        auto map = std::make_shared<Data::ElementMap>()->restore(_hasher, input);
        EXPECT_EQ(
            map->find(Data::IndexedName("Face", 1)).toString(),
            value == 0 ? "Face" : "Face" + std::to_string(value)
        );
    }
}

TEST_F(ElementMapTest, restoreSaturatedIdsDoNotAliasValidIds)
{
    auto sid = _hasher->getID("range-control");
    const std::string oversizedHex = "1" + std::string(sizeof(long) * 2 - 1, '0') + "1";
    const std::string oversizedDec(4096, '9');
    std::stringstream input;
    input << "988200 PostfixCount 1\n;Unexpected\nMapCount 1\nElementMap 1 988200 1\nFace\n"
          << "ChildCount 1\n3 0 2 7 0 ChildPostfix 0." << oversizedDec << '.' << sid.value()
          << "\nNameCount 2\n0\n;Raw." << oversizedHex << '.' << oversizedHex << '.'
          << formatRestoreLong(sid.value(), 16) << " 0\nEndMap\n";
    auto map = std::make_shared<Data::ElementMap>()->restore(_hasher, input);
    Data::ElementIDRefs ids;
    EXPECT_EQ(map->find(Data::IndexedName("Face", 1), &ids).toString(), "Raw");
    ASSERT_EQ(ids.size(), 1);
    EXPECT_EQ(ids.front().value(), sid.value());
    auto children = map->getChildElements();
    ASSERT_EQ(children.size(), 1);
    ASSERT_EQ(children.front().sids.size(), 1);
    EXPECT_EQ(children.front().sids.front().value(), sid.value());
}

TEST_F(ElementMapTest, restoreRejectsInvalidChildRanges)
{
    const std::vector<std::string> ranges {
        std::to_string(INT_MAX) + " 1 1",
        "0 " + std::to_string(INT_MAX) + " 1",
        "1 0 -1"
    };
    unsigned id = 988300;
    for (const auto& range : ranges) {
        SCOPED_TRACE(range);
        ++id;
        std::stringstream input;
        input << id << " PostfixCount 0\nMapCount 1\nElementMap 1 " << id
              << " 1\nFace\nChildCount 1\n"
              << range << " 7 0 ChildPostfix 0\nNameCount 0\nEndMap\n";
        EXPECT_THROW(std::make_shared<Data::ElementMap>()->restore(_hasher, input), Base::RuntimeError);
    }
}

TEST_F(ElementMapTest, restoreRejectsNegativeMetadataCounts)
{
    const std::vector<std::string> inputs {
        "988400 PostfixCount -1\n",
        "988401 PostfixCount 0\nMapCount -1\n",
        "988402 PostfixCount 0\nMapCount 1\nElementMap 1 988402 1\nFace\nChildCount -1\nNameCount "
        "0\nEndMap\n",
        "988403 PostfixCount 0\nMapCount 1\nElementMap 1 988403 1\nFace\nChildCount 0\nNameCount "
        "-1\nEndMap\n"
    };
    for (const auto& text : inputs) {
        SCOPED_TRACE(text);
        std::stringstream input(text);
        EXPECT_THROW(std::make_shared<Data::ElementMap>()->restore(_hasher, input), Base::RuntimeError);
    }
}

namespace
{
std::vector<long> sidValues(const Data::ElementIDRefs& refs)
{
    std::vector<long> values;
    for (const auto& ref : refs) {
        values.push_back(ref.value());
    }
    return values;
}

Data::ElementMapPtr restoreOwnedMap(const std::string& archive, App::StringHasherRef hasher)
{
    // Neither the stream nor any of its backing strings survive this helper.
    std::istringstream input(archive);
    return std::make_shared<Data::ElementMap>()->restore(hasher, input);
}

void expectSameMap(const Data::ElementMapPtr& expected, const Data::ElementMapPtr& actual)
{
    auto entries = expected->getAll();
    ASSERT_EQ(actual->getAll().size(), entries.size());
    for (const auto& entry : entries) {
        SCOPED_TRACE(entry.name.toString());
        Data::ElementIDRefs before, after;
        EXPECT_EQ(expected->find(entry.name, &before), entry.index);
        EXPECT_EQ(actual->find(entry.name, &after), entry.index);
        EXPECT_EQ(sidValues(before), sidValues(after));
        EXPECT_EQ(actual->find(entry.index), expected->find(entry.index));
        const auto aliases = expected->findAll(entry.index);
        const auto restoredAliases = actual->findAll(entry.index);
        ASSERT_EQ(aliases.size(), restoredAliases.size());
        for (std::size_t i = 0; i < aliases.size(); ++i) {
            EXPECT_EQ(aliases[i].first, restoredAliases[i].first);
            EXPECT_EQ(sidValues(aliases[i].second), sidValues(restoredAliases[i].second));
        }
    }
    const auto before = expected->getChildElements();
    const auto after = actual->getChildElements();
    ASSERT_EQ(before.size(), after.size());
    // Child iteration is a QHash; compare by postfix rather than iteration order.
    for (const auto& child : before) {
        auto found = std::find_if(after.begin(), after.end(), [&](const auto& other) {
            return child.postfix == other.postfix;
        });
        ASSERT_NE(found, after.end());
        EXPECT_EQ(found->indexedName, child.indexedName);
        EXPECT_EQ(found->count, child.count);
        EXPECT_EQ(found->offset, child.offset);
        EXPECT_EQ(found->tag, child.tag);
        EXPECT_EQ(sidValues(found->sids), sidValues(child.sids));
        ASSERT_EQ(bool(found->elementMap), bool(child.elementMap));
        if (child.elementMap) {
            expectSameMap(child.elementMap, found->elementMap);
        }
    }
}

std::string saveMap(const Data::ElementMapPtr& map, App::StringHasherRef hasher)
{
    map->beforeSave(hasher);
    std::ostringstream output;
    map->save(output);
    return output.str();
}
}  // namespace

TEST_F(ElementMapTest, writerFormsAliasesAndMultipleSidsRoundTrip)
{
    std::vector<App::StringIDRef> ids;
    for (int i = 0; i < 20; ++i) {
        ids.push_back(_hasher->getID(("identity-sid-" + std::to_string(i)).c_str()));
    }
    ASSERT_GT(ids[10].value(), 9);
    auto original = std::make_shared<Data::ElementMap>();
    original->hasher = _hasher;
    const Data::ElementIDRefs refs {ids[10], ids[12]};
    Data::MappedName literal("CustomMapped");
    literal += ";Postfix";
    original->setElementName(Data::IndexedName("Edge", 1), literal, 0, &refs);
    original->setElementName(Data::IndexedName("Edge", 1), Data::MappedName("Alias"), 0, &refs);
    original->setElementName(Data::IndexedName("Face", 1), Data::MappedName("Face12"), 0, &refs);
    Data::MappedName hashed(ids[10].toString());
    hashed += ";HashPostfix";
    original->setElementName(Data::IndexedName("Edge", 2), hashed, 0, &refs);
    const auto first = saveMap(original, _hasher);
    EXPECT_NE(first.find(";CustomMapped."), std::string::npos);
    EXPECT_NE(first.find(":"), std::string::npos);
    EXPECT_NE(first.find("$" + ids[10].toString() + "."), std::string::npos);
    std::ostringstream sidSuffix;
    sidSuffix << std::hex << '.' << ids[10].value() << '.' << ids[12].value();
    EXPECT_NE(first.find(sidSuffix.str()), std::string::npos);
    auto restored = restoreOwnedMap(first, _hasher);
    // Churn freed parser buffers before checking both lookup directions and SIDs.
    const std::vector<std::string> churn(100, std::string(4096, 'x'));
    expectSameMap(original, restored);
    const auto second = saveMap(restored, _hasher);
    auto twice = restoreOwnedMap(second, _hasher);
    expectSameMap(restored, twice);
    EXPECT_EQ(twice->findAll(Data::IndexedName("Edge", 1)).size(), 2);
}

TEST_F(ElementMapTest, nestedSharedChildrenAndDecimalSidsRoundTrip)
{
    std::vector<App::StringIDRef> ids;
    for (int i = 0; i < 20; ++i) {
        ids.push_back(_hasher->getID(("child-identity-" + std::to_string(i)).c_str()));
    }
    std::ostringstream archive;
    archive
        << "991600 PostfixCount 1\nFace\nMapCount 3\n"
        << "ElementMap 1 991601 1\nFace\nChildCount 0\nNameCount 3\n0\n:1.1.0 0\n:1.2.0 0\nEndMap\n"
        << "ElementMap 2 991602 1\nFace\nChildCount 1\n"
        << "1 0 2 21 1 ;:H15:0,F 0." << ids[10].value() << '.' << ids[12].value()
        << "\nNameCount 0\nEndMap\n"
        << "ElementMap 3 991600 1\nFace\nChildCount 2\n"
        << "1 0 2 22 2 ;:H16:0,F 0." << ids[10].value() << '\n'
        << "1 2 2 23 2 ;:H17:0,F 0." << ids[12].value() << "\nNameCount 0\nEndMap\n";
    auto original = restoreOwnedMap(archive.str(), _hasher);
    const auto children = original->getChildElements();
    ASSERT_EQ(children.size(), 2);
    ASSERT_EQ(children[0].elementMap, children[1].elementMap);
    const auto nested = children[0].elementMap->getChildElements();
    ASSERT_EQ(nested.size(), 1);
    EXPECT_EQ(sidValues(nested[0].sids), (std::vector<long> {ids[10].value(), ids[12].value()}));
    auto saved = saveMap(original, _hasher);
    std::ostringstream decimal;
    decimal << "0." << ids[10].value() << '.' << ids[12].value();
    EXPECT_NE(saved.find(decimal.str()), std::string::npos);
    auto restored = restoreOwnedMap(saved, _hasher);
    expectSameMap(original, restored);
    const auto restoredChildren = restored->getChildElements();
    ASSERT_EQ(restoredChildren.size(), 2);
    EXPECT_EQ(restoredChildren[0].elementMap, restoredChildren[1].elementMap);
    EXPECT_EQ(restored->getAll().size(), 4);
    auto twice = restoreOwnedMap(saveMap(restored, _hasher), _hasher);
    expectSameMap(restored, twice);
}

TEST_F(ElementMapTest, restoreChildSidsWithoutHasher)
{
    auto restored = restoreOwnedMap(
        "991610 PostfixCount 0\nMapCount 1\nElementMap 1 991610 1\nFace\nChildCount 1\n"
        "1 0 2 0 0 Child 0.11.13\nNameCount 0\nEndMap\n",
        {}
    );
    const auto children = restored->getChildElements();
    ASSERT_EQ(children.size(), 1);
    EXPECT_TRUE(children[0].sids.empty());
    EXPECT_EQ(children[0].count, 2);
}

TEST_F(ElementMapTest, restoreMissingAndMalformedChildSids)
{
    auto sid = _hasher->getID("valid-child");
    std::ostringstream archive;
    archive << "991611 PostfixCount 0\nMapCount 1\nElementMap 1 991611 1\nFace\nChildCount 1\n"
            << "1 0 2 0 0 Child 0.999999.bad..-1." << sid.value() << "\nNameCount 0\nEndMap\n";
    auto restored = restoreOwnedMap(archive.str(), _hasher);
    const auto children = restored->getChildElements();
    ASSERT_EQ(children.size(), 1);
    EXPECT_EQ(sidValues(children[0].sids), (std::vector<long> {sid.value()}));
}

// NOLINTEND(readability-magic-numbers)
