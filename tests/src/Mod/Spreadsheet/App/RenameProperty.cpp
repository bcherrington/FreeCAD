// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include <gtest/gtest.h>
#include <memory>
#include <algorithm>

#include <Mod/Spreadsheet/App/PropertySheet.h>
#include <Mod/Spreadsheet/App/Sheet.h>

#include "src/App/Property.h"

class SpreadsheetRenameProperty: public RenameProperty
{
protected:
    void SetUp() override
    {
        RenameProperty::SetUp();
        sheet = freecad_cast<Spreadsheet::Sheet*>(doc->addObject("Spreadsheet::Sheet", "Sheet"));
    }

    void TearDown() override
    {
        doc->removeObject(sheet->getNameInDocument());
        RenameProperty::TearDown();
    }

    Spreadsheet::Sheet* sheet {};
};

App::Document* RenameProperty::doc {nullptr};

// Tests whether we can rename a property that is used in a spreadsheet
TEST_F(SpreadsheetRenameProperty, renameProperty)
{
    // Arrange
    sheet->setCell("A1", "=VarSet.Variable");
    doc->recompute();
    auto* propSpreadsheet = static_cast<App::PropertyInteger*>(sheet->getPropertyByName("A1"));

    // Assert before the rename
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(propSpreadsheet->getValue(), value);

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName");
    doc->recompute();

    // Assert after the rename
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), value);

    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);

    std::string cellContent;
    sheet->getCell(App::CellAddress("A1"))->getStringContent(cellContent);

    EXPECT_EQ(cellContent, "=VarSet.NewName");
    EXPECT_EQ(propSpreadsheet->getValue(), value);
}

TEST_F(SpreadsheetRenameProperty, copiedCellsFollowPropertyRename)
{
    sheet->setCell("A1", "=VarSet.Variable");
    doc->recompute();
    auto* cells = dynamic_cast<Spreadsheet::PropertySheet*>(sheet->getPropertyByName("cells"));
    ASSERT_NE(cells, nullptr);
    std::unique_ptr<App::Property, decltype(&App::Property::destroy)> copy(
        cells->Copy(),
        &App::Property::destroy
    );
    auto* copiedCells = dynamic_cast<Spreadsheet::PropertySheet*>(copy.get());
    ASSERT_NE(copiedCells, nullptr);
    ASSERT_TRUE(varSet->renameDynamicProperty(prop, "NewName"));
    doc->recompute();
    std::string liveContent;
    std::string copiedContent;
    sheet->getCell(App::CellAddress("A1"))->getStringContent(liveContent);
    auto* copiedCell = copiedCells->getValue(App::CellAddress("A1"));
    ASSERT_NE(copiedCell, nullptr);
    copiedCell->getStringContent(copiedContent);
    EXPECT_EQ(liveContent, "=VarSet.NewName");
    EXPECT_EQ(copiedContent, "=VarSet.NewName");
}

TEST_F(SpreadsheetRenameProperty, transformedCopyReleasesOldTarget)
{
    auto* replacement = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Replacement"));
    auto* replacementProperty = freecad_cast<App::PropertyInteger*>(
        replacement->addDynamicProperty("App::PropertyInteger", "Variable", "Variables")
    );
    replacementProperty->setValue(value);
    sheet->setCell("A1", "=VarSet.Variable");
    doc->recompute();
    auto* cells = dynamic_cast<Spreadsheet::PropertySheet*>(sheet->getPropertyByName("cells"));
    ASSERT_NE(cells, nullptr);
    std::unique_ptr<App::Property, decltype(&App::Property::destroy)> copy(
        cells->CopyOnLinkReplace(sheet, varSet, replacement),
        &App::Property::destroy
    );
    auto* copiedCells = dynamic_cast<Spreadsheet::PropertySheet*>(copy.get());
    ASSERT_NE(copiedCells, nullptr);
    std::vector<App::DocumentObject*> endpoints;
    copiedCells->collectAdmissionTargets(endpoints);
    EXPECT_EQ(std::find(endpoints.begin(), endpoints.end(), varSet), endpoints.end());
    EXPECT_NE(std::find(endpoints.begin(), endpoints.end(), replacement), endpoints.end());
    doc->removeObject(varSet->getNameInDocument());
    // Keep the fixture cleanup attached to the surviving replacement.
    varSet = replacement;
    prop = replacementProperty;
    ASSERT_TRUE(varSet->renameDynamicProperty(prop, "NewName"));
    std::string content;
    auto* copiedCell = copiedCells->getValue(App::CellAddress("A1"));
    ASSERT_NE(copiedCell, nullptr);
    copiedCell->getStringContent(content);
    EXPECT_EQ(content, "=Replacement.NewName");
}
