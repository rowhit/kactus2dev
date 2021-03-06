//-----------------------------------------------------------------------------
// File: registerfilegraphitem.cpp
//-----------------------------------------------------------------------------
// Project: Kactus2
// Author: Dan Chianucci
// Date: 19.06.2018
//
// Description:
// Graphical item to represent register file in visualization.
//-----------------------------------------------------------------------------
#include "registerfilegraphitem.h"

#include <editors/ComponentEditor/common/ExpressionParser.h>
#include <editors/ComponentEditor/visualization/memorygapitem.h>

#include <common/KactusColors.h>

#include <QBrush>

//-----------------------------------------------------------------------------
// Function: RegisterFileGraphItem::RegisterFileGraphItem()
//-----------------------------------------------------------------------------
RegisterFileGraphItem::RegisterFileGraphItem(QSharedPointer<RegisterFile> regFile,
    QSharedPointer<ExpressionParser> expressionParser,
    QGraphicsItem *parent) :
    MemoryVisualizationItem(expressionParser, parent),
    regFile_(regFile)
{
    Q_ASSERT(regFile_);
    QBrush brush(KactusColors::REGISTER_FILE_COLOR);
    setDefaultBrush(brush);
    updateDisplay();
}

//-----------------------------------------------------------------------------
// Function: RegisterFileGraphItem::setDimensionIndex()
//-----------------------------------------------------------------------------
void RegisterFileGraphItem::setDimensionIndex(unsigned int index)
{
    dimensionIndex_ = index;
}

//-----------------------------------------------------------------------------
// Function: RegisterFileGraphItem::refresh()
//-----------------------------------------------------------------------------
void RegisterFileGraphItem::refresh()
{
    updateDisplay();
    reorganizeChildren();
}

//-----------------------------------------------------------------------------
// Function: RegisterFileGraphItem::updateDisplay()
//-----------------------------------------------------------------------------
void RegisterFileGraphItem::updateDisplay()
{
    QString name = regFile_->name();
    if (parseExpression(regFile_->getDimension()) > 0)
    {
        name.append("[" + QString::number(dimensionIndex_) + "]");
    }

    quint64 offset = getOffset();
    quint64 lastAddress = getLastAddress();

    setName(name);
    setDisplayOffset(offset);
    setDisplayLastAddress(lastAddress);

    // Set tooltip to show addresses in hexadecimals.
    setToolTip("<b>Name: </b>" + regFile_->name() + "<br>" +
        "<b>AUB: </b>" + QString::number(getAddressUnitSize()) + "<br>" +
        "<b>First address: </b>" + toHexString(offset) + "<br>" +
        "<b>Last address: </b>" + toHexString(lastAddress));
}

//-----------------------------------------------------------------------------
// Function: RegisterFileGraphItem::getOffset()
//-----------------------------------------------------------------------------
quint64 RegisterFileGraphItem::getOffset() const
{
    // the register offset from the address block
    quint64 regFileOffset = parseExpression(regFile_->getAddressOffset());

    // the address block's offset
    MemoryVisualizationItem* parent = static_cast<MemoryVisualizationItem*>(parentItem());
    Q_ASSERT(parent);
    quint64 parentOffset = parent->getOffset();
    quint64 range = parseExpression(regFile_->getRange());

    // the total offset is the address block's offset added with register's offset
    return parentOffset + regFileOffset + (dimensionIndex_ * range);
}

//-----------------------------------------------------------------------------
// Function: RegisterFileGraphItem::getLastAddress()
//-----------------------------------------------------------------------------
quint64 RegisterFileGraphItem::getLastAddress() const
{
    quint64 base = getOffset();
    quint64 range = parseExpression(regFile_->getRange());

    quint64 lastAddr = base + range;

    if (lastAddr == 0)
    {
        return 0;
    }
    return lastAddr - 1;
}

//-----------------------------------------------------------------------------
// Function: RegisterFileGraphItem::getBitWidth()
//-----------------------------------------------------------------------------
int RegisterFileGraphItem::getBitWidth() const
{
    MemoryVisualizationItem* parent = static_cast<MemoryVisualizationItem*>(parentItem());
    return parent->getBitWidth();
}

//-----------------------------------------------------------------------------
// Function: RegisterFileGraphItem::getAddressUnitSize()
//-----------------------------------------------------------------------------
unsigned int RegisterFileGraphItem::getAddressUnitSize() const
{
    MemoryVisualizationItem* parent = static_cast<MemoryVisualizationItem*>(parentItem());
    Q_ASSERT(parent);
    return parent->getAddressUnitSize();
}

//-----------------------------------------------------------------------------
// Function: RegisterFileGraphItem::isPresent()
//-----------------------------------------------------------------------------
bool RegisterFileGraphItem::isPresent() const
{
    return regFile_->getIsPresent().isEmpty() || parseExpression(regFile_->getIsPresent()) == 1;
}
