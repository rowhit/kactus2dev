//-----------------------------------------------------------------------------
// File: MemoryDesignerDiagram.cpp
//-----------------------------------------------------------------------------
// Project: Kactus2
// Author: Mikko Teuho
// Date: 07.07.2016
//
// Description:
// Declares the memory design diagram class.
//-----------------------------------------------------------------------------

#include "MemoryDesignerDiagram.h"

#include <library/LibraryManager/libraryinterface.h>

#include <common/graphicsItems/GraphicsColumnLayout.h>

#include <designEditors/common/diagramgrid.h>

#include <designEditors/MemoryDesigner/MemoryDesignDocument.h>
#include <designEditors/MemoryDesigner/MasterSlavePathSearch.h>
#include <designEditors/MemoryDesigner/ConnectivityGraph.h>
#include <designEditors/MemoryDesigner/ConnectivityConnection.h>
#include <designEditors/MemoryDesigner/ConnectivityInterface.h>
#include <designEditors/MemoryDesigner/AddressSpaceGraphicsItem.h>
#include <designEditors/MemoryDesigner/MemoryMapGraphicsItem.h>
#include <designEditors/MemoryDesigner/MainMemoryGraphicsItem.h>
#include <designEditors/MemoryDesigner/MemoryExtensionGraphicsItem.h>
#include <designEditors/MemoryDesigner/MemoryConnectionItem.h>
#include <designEditors/MemoryDesigner/ConnectivityGraph.h>
#include <designEditors/MemoryDesigner/ConnectivityComponent.h>
#include <designEditors/MemoryDesigner/MemoryItem.h>
#include <designEditors/MemoryDesigner/MemoryCollisionItem.h>
#include <designEditors/MemoryDesigner/MemoryColumn.h>
#include <designEditors/MemoryDesigner/MemoryDesignerConstants.h>

#include <IPXACTmodels/kactusExtensions/ColumnDesc.h>
#include <IPXACTmodels/common/VLNV.h>
#include <IPXACTmodels/Component/Component.h>
#include <IPXACTmodels/Design/Design.h>
#include <IPXACTmodels/designConfiguration/DesignConfiguration.h>

#include <QPainter>
#include <QPen>
#include <QGraphicsSceneWheelEvent>

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::MemoryDesignerDiagram()
//-----------------------------------------------------------------------------
MemoryDesignerDiagram::MemoryDesignerDiagram(LibraryInterface* library, MemoryDesignDocument* parent):
QGraphicsScene(parent),
parentDocument_(parent),
layout_(new GraphicsColumnLayout(this)),
libraryHandler_(library),
instanceLocator_(library),
condenseMemoryItems_(true),
filterAddressSpaceChains_(false),
filterAddressSpaceSegments_(false),
filterAddressBlocks_(false),
filterRegisters_(false),
filterFields_(false),
spaceColumnWidth_(MemoryDesignerConstants::SPACECOLUMNWIDTH),
memoryMapColumnWidth_(MemoryDesignerConstants::MEMORYMAPCOLUMNWIDTH),
widthBoundary_(0),
connectionsToMemoryMaps_(),
memoryCollisions_()
{
    setSceneRect(0, 0, 100000, 100000);
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::~MemoryDesignDocument()
//-----------------------------------------------------------------------------
MemoryDesignerDiagram::~MemoryDesignerDiagram()
{

}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::setCondenseMemoryItems()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::setCondenseMemoryItems(bool condenseMemoryItems)
{
    condenseMemoryItems_ = condenseMemoryItems;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::memoryItemsAreCondensed()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::memoryItemsAreCondensed() const
{
    return condenseMemoryItems_;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::setFilterAddressSpaceChains()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::setFilterAddressSpaceChains(bool filterChains)
{
    filterAddressSpaceChains_ = filterChains;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::addressSpaceChainsAreFiltered()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::addressSpaceChainsAreFiltered() const
{
    return filterAddressSpaceChains_;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::setFilterAddressSpaceSegments()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::setFilterAddressSpaceSegments(bool filterSegments)
{
    filterAddressSpaceSegments_ = filterSegments;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::addressSpaceSegmentsAreFiltered()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::addressSpaceSegmentsAreFiltered() const
{
    return filterAddressSpaceSegments_;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::setFilterAddressBlocks()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::setFilterAddressBlocks(bool filterBlocks)
{
    filterAddressBlocks_ = filterBlocks;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::addressBlocksAreFiltered()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::addressBlocksAreFiltered() const
{
    return filterAddressBlocks_;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::setFilterAddressBlockRegisters()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::setFilterAddressBlockRegisters(bool filterRegisters)
{
    filterRegisters_ = filterRegisters;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::addressBlockRegistersAreFiltered()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::addressBlockRegistersAreFiltered() const
{
    return filterRegisters_;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::setFilterFields()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::setFilterFields(bool filterFields)
{
    filterFields_ = filterFields;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::fieldsAreFiltered()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::fieldsAreFiltered() const
{
    return filterFields_;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::loadDesign()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::loadDesignFromCurrentView(QSharedPointer<const Component> component,
    QString const& viewName)
{
    clearScene();
    connectionsToMemoryMaps_.clear();
    memoryCollisions_.clear();

    if (filterRegisters_ || (filterAddressBlocks_ && filterFields_))
    {
        memoryMapColumnWidth_ = MemoryDesignerConstants::SPACECOLUMNWIDTH;
    }
    else if (filterFields_)
    {
        memoryMapColumnWidth_ =
            MemoryDesignerConstants::SPACECOLUMNWIDTH + (MemoryDesignerConstants::MAPSUBITEMPOSITIONX * 2 - 5);
    }
    else if (filterAddressBlocks_)
    {
        memoryMapColumnWidth_ =
            MemoryDesignerConstants::MEMORYMAPCOLUMNWIDTH - (MemoryDesignerConstants::MAPSUBITEMPOSITIONX * 2 - 5);
    }
    else
    {
        memoryMapColumnWidth_ = MemoryDesignerConstants::MEMORYMAPCOLUMNWIDTH;
    }

    if (createConnectionGraph(component, viewName))
    {
        createInitialColumns();
        createMemoryItems();
        createMemoryConnections();

        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::clearScene()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::clearScene()
{
    clearLayout();
    clear();
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::clearLayout()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::clearLayout()
{
    layout_.clear();
    layout_ = QSharedPointer<GraphicsColumnLayout>(new GraphicsColumnLayout(this));
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createConnectionGraph()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::createConnectionGraph(QSharedPointer<const Component> component,
    QString const& viewName)
{
    QSharedPointer<Design> selectedDesign = getContainingDesign(component, viewName);
    if (selectedDesign)
    {
        QSharedPointer<const DesignConfiguration> containingConfiguration =
            getContainingDesignConfiguration(component, viewName);
        if (containingConfiguration)
        {
            connectionGraph_ = instanceLocator_.createConnectivityGraph(selectedDesign, containingConfiguration);
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::getDesignFromComponentUsingView()
//-----------------------------------------------------------------------------
QSharedPointer<Design> MemoryDesignerDiagram::getContainingDesign(QSharedPointer<const Component> component,
    QString const& viewName) const
{
    foreach (QSharedPointer<View> componentView, *component->getViews())
    {
        if (componentView->name() == viewName)
        {
            if (componentView->isHierarchical())
            {
                VLNV designVLNV;

                QString designInstantiationReference = componentView->getDesignInstantiationRef();
                if (designInstantiationReference.isEmpty())
                {
                    QSharedPointer<const DesignConfiguration> containingConfiguration =
                        getContainingDesignConfiguration(component, viewName);
                    if (containingConfiguration)
                    {
                        designVLNV = containingConfiguration->getDesignRef();
                    }
                }
                else
                {
                    foreach (QSharedPointer<DesignInstantiation> instantiation,
                        *component->getDesignInstantiations())
                    {
                        if (instantiation->name() == viewName)
                        {
                            designVLNV = *instantiation->getDesignReference().data();
                            break;
                        }
                    }
                }

                if (designVLNV.isValid() && designVLNV.getType() == VLNV::DESIGN)
                {
                    QSharedPointer<Design> containingDesign = libraryHandler_->getDesign(designVLNV);
                    return containingDesign;
                }
            }
            else
            {
                break;
            }
        }
    }

    return QSharedPointer<Design>();
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::getContainingDesignConfiguration()
//-----------------------------------------------------------------------------
QSharedPointer<const DesignConfiguration> MemoryDesignerDiagram::getContainingDesignConfiguration(
    QSharedPointer<const Component> component, QString const& viewName) const
{
    foreach (QSharedPointer<View> activeView, *component->getViews())
    {
        if (activeView->name() == viewName)
        {
            QString designConfigurationReference = activeView->getDesignConfigurationInstantiationRef();
            if (!designConfigurationReference.isEmpty())
            {
                foreach (QSharedPointer<DesignConfigurationInstantiation> instantiation,
                    *component->getDesignConfigurationInstantiations())
                {
                    if (instantiation->name() == designConfigurationReference)
                    {
                        QSharedPointer<ConfigurableVLNVReference> designConfigurationVLNV =
                            instantiation->getDesignConfigurationReference();

                        QSharedPointer<const Document> designConfigurationDocument =
                            libraryHandler_->getModelReadOnly(*designConfigurationVLNV.data());
                        QSharedPointer<const DesignConfiguration> designConfiguration =
                            designConfigurationDocument.dynamicCast<const DesignConfiguration>();

                        return designConfiguration;
                    }
                }
            }
        }
    }

    return QSharedPointer<const DesignConfiguration>();
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createInitialColumns()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::createInitialColumns()
{
    createAddressSpaceColumn();
    createMemoryColumns();
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createAddressSpaceColumn()
//-----------------------------------------------------------------------------
MemoryColumn* MemoryDesignerDiagram::createAddressSpaceColumn()
{
    QString const SPACENAME = MemoryDesignerConstants::ADDRESSSPACECOLUMN_NAME;

    QSharedPointer<ColumnDesc> addressSpaceColumnDescription(new ColumnDesc());
    addressSpaceColumnDescription->setName(SPACENAME);
    addressSpaceColumnDescription->setMinimumWidth(spaceColumnWidth_);
    addressSpaceColumnDescription->setWidth(spaceColumnWidth_);

    QPointF columnPosition (0,0);

    int spaceCounter = 0;
    foreach (GraphicsColumn* column, layout_->getColumns())
    {
        if (column->name().contains(SPACENAME, Qt::CaseInsensitive))
        {
            spaceCounter += 1;
        }
    }
    if (spaceCounter != 0)
    {
        addressSpaceColumnDescription->setName(SPACENAME + "_" + QString::number(spaceCounter));
    }

    MemoryColumn* spaceColumn(new MemoryColumn(addressSpaceColumnDescription, layout_.data(), 0));
    layout_->addColumn(spaceColumn);

    spaceColumn->setPos(columnPosition);

    layout_->onMoveColumn(spaceColumn);
    layout_->onReleaseColumn(spaceColumn);

    return spaceColumn;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createMemoryColumns()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::createMemoryColumns()
{
    QPointF columnPlacement (0,0);
    foreach (GraphicsColumn* column, layout_->getColumns())
    {
        columnPlacement.setX(columnPlacement.x() + column->boundingRect().width());
    }

    QSharedPointer<ColumnDesc> memoryMapColumnDescription(new ColumnDesc());
    memoryMapColumnDescription->setName(MemoryDesignerConstants::MEMORYMAPCOLUM_NAME);
    memoryMapColumnDescription->setMinimumWidth(memoryMapColumnWidth_);
    memoryMapColumnDescription->setWidth(memoryMapColumnWidth_);

    MemoryColumn* memoryMapColumn(new MemoryColumn(memoryMapColumnDescription, layout_.data(), 0));

    memoryMapColumn->setPos(columnPlacement);
    layout_->addColumn(memoryMapColumn);

    createMemoryOverlapColumn();
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createMemoryOverlapColumn()
//-----------------------------------------------------------------------------
MemoryColumn* MemoryDesignerDiagram::createMemoryOverlapColumn()
{
    QString overlapName = MemoryDesignerConstants::MEMORYMAPOVERLAPCOLUMN_NAME;

    QSharedPointer<ColumnDesc> memoryMapOverlapColumnDescription(new ColumnDesc());
    memoryMapOverlapColumnDescription->setName(overlapName);
    memoryMapOverlapColumnDescription->setMinimumWidth(memoryMapColumnWidth_);
    memoryMapOverlapColumnDescription->setWidth(memoryMapColumnWidth_);

    int overlapCounter = 0;
    int columnXPosition = 0;
    foreach (GraphicsColumn* column, layout_->getColumns())
    {
        if (column->name().contains(overlapName))
        {
            overlapCounter += 1;
        }

        columnXPosition += column->boundingRect().width();
    }

    if (overlapCounter > 0)
    {
        memoryMapOverlapColumnDescription->setName(overlapName + "_" + QString::number(overlapCounter));
    }

    MemoryColumn* memoryMapOverlapColumn(new MemoryColumn(memoryMapOverlapColumnDescription, layout_.data(), 0));
    QPointF columnPosition (columnXPosition, 0);
    memoryMapOverlapColumn->setPos(columnPosition);
    layout_->addColumn(memoryMapOverlapColumn);

    return memoryMapOverlapColumn;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createMemoryItems()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::createMemoryItems()
{
    MemoryColumn* addressSpaceColumn = findColumnByName(MemoryDesignerConstants::ADDRESSSPACECOLUMN_NAME);
    MemoryColumn* memoryMapColumn = findColumnByName(MemoryDesignerConstants::MEMORYMAPCOLUM_NAME);
    
    if (addressSpaceColumn && memoryMapColumn)
    {
        foreach (QSharedPointer<ConnectivityComponent> componentInstance, connectionGraph_->getInstances())
        {
            foreach (QSharedPointer<MemoryItem> memoryItem, componentInstance->getMemories())
            {
                if (memoryItem->getType().compare("addressSpace", Qt::CaseInsensitive) == 0)
                {
                    createAddressSpaceItem(memoryItem, componentInstance, addressSpaceColumn);
                }
                else if (memoryItem->getType().compare("memoryMap", Qt::CaseInsensitive) == 0)
                {
                    createMemoryMapItem(memoryItem, componentInstance, memoryMapColumn);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createAddressSpaceItem()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::createAddressSpaceItem(QSharedPointer<MemoryItem> spaceItem,
    QSharedPointer<ConnectivityComponent> containingInstance, MemoryColumn* containingColumn)
{
    if (containingColumn)
    {
        AddressSpaceGraphicsItem* spaceGraphicsItem = new AddressSpaceGraphicsItem(
            spaceItem, containingInstance, filterAddressSpaceSegments_, containingColumn);
        containingColumn->addItem(spaceGraphicsItem);
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createMemoryMapItem()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::createMemoryMapItem(QSharedPointer<MemoryItem> mapItem,
    QSharedPointer<ConnectivityComponent> containingInstance, MemoryColumn* containingColumn)
{
    if (containingColumn)
    {
        MemoryMapGraphicsItem* mapGraphicsItem = new MemoryMapGraphicsItem(
            mapItem, filterAddressBlocks_, filterRegisters_, filterFields_, containingInstance, containingColumn);
        containingColumn->addItem(mapGraphicsItem);
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createMemoryConnections()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::createMemoryConnections()
{
    MasterSlavePathSearch pathSearcher;

    QVector<QVector<QSharedPointer<ConnectivityInterface> > > masterSlavePaths =
        pathSearcher.findMasterSlavePaths(connectionGraph_);

    QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedMapItems(
        new QVector<MainMemoryGraphicsItem*> ());

    int spaceYPlacement = MemoryDesignerConstants::SPACEITEMINTERVAL;

    QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedSpaceItems(
        new QVector<MainMemoryGraphicsItem*> ());

    MemoryColumn* spaceColumn = findColumnByName(MemoryDesignerConstants::ADDRESSSPACECOLUMN_NAME);
    MemoryColumn* memoryMapColumn = findColumnByName(MemoryDesignerConstants::MEMORYMAPCOLUM_NAME);

    foreach (QVector<QSharedPointer<ConnectivityInterface> > singlePath, masterSlavePaths)
    {
        createConnection(
            singlePath, placedMapItems, memoryMapColumn, spaceYPlacement, placedSpaceItems, spaceColumn);
    }

    compressGraphicsItems(placedSpaceItems, spaceYPlacement, spaceColumn);
    if (condenseMemoryItems_)
    {
        repositionCompressedMemoryMaps(placedMapItems, memoryMapColumn);
    }

    moveUnconnectedAddressSpaces(placedSpaceItems, spaceYPlacement, spaceColumn);
    moveUnconnectedMemoryMaps(placedMapItems, memoryMapColumn);

    reDrawConnections(placedSpaceItems);

    createOverlappingConnectionMarkers(placedSpaceItems);
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createConnection()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::createConnection(QVector<QSharedPointer<ConnectivityInterface> > connectionPath,
    QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedMapItems, MemoryColumn* memoryMapColumn,
    int& spaceYPlacement, QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedSpaceItems,
    MemoryColumn* spaceColumn)
{
    QSharedPointer<ConnectivityInterface> startInterface = connectionPath.first();
    QSharedPointer<ConnectivityInterface> endInterface = connectionPath.last();

    if (startInterface && endInterface && startInterface != endInterface)
    {
        MainMemoryGraphicsItem* connectionStartItem =
            getMainGraphicsItem(startInterface, MemoryDesignerConstants::ADDRESSSPACECOLUMN_NAME);
        MainMemoryGraphicsItem* connectionEndItem =
            getMainGraphicsItem(endInterface, MemoryDesignerConstants::MEMORYMAPCOLUMNCOMMON_NAME);
        if (connectionStartItem && connectionEndItem)
        {
            connectionStartItem->hideMemoryRangeLabels();

            if (!placedSpaceItems->contains(connectionStartItem))
            {
                moveAddressSpaceItem(connectionStartItem, spaceColumn, spaceYPlacement);
            }

            quint64 baseAddressNumber = startInterface->getBaseAddress().toULongLong();

            bool hasRemapRange = false;
            quint64 mirroredSlaveAddressChange = 0;
            quint64 memoryMapEndAddress = 0;

            QVector<MainMemoryGraphicsItem*> spaceItemChain;
            spaceItemChain.append(connectionStartItem);

            QVector<MemoryConnectionItem*> connectionChain;

            foreach(QSharedPointer<ConnectivityInterface> pathInterface, connectionPath)
            {
                if (pathInterface != startInterface && pathInterface != endInterface)
                {
                    if (pathInterface->getMode().compare("mirroredSlave", Qt::CaseInsensitive) == 0)
                    {
                        mirroredSlaveAddressChange += pathInterface->getRemapAddress().toULongLong();

                        memoryMapEndAddress = pathInterface->getRemapRange().toULongLong() - 1;
                        hasRemapRange = true;
                    }
                    else if (pathInterface->getMode().compare("master", Qt::CaseInsensitive) == 0)
                    {
                        if (pathInterface->isConnectedToMemory())
                        {
                            MainMemoryGraphicsItem* connectionMiddleItem = getMainGraphicsItem(
                                pathInterface, MemoryDesignerConstants::ADDRESSSPACECOLUMN_NAME);
                            if (connectionMiddleItem)
                            {
                                if (filterAddressSpaceChains_)
                                {
                                    connectionMiddleItem->hide();
                                }
                                else
                                {
                                    MemoryConnectionItem* newSpaceConnection = createSpaceConnection(
                                        connectionStartItem, connectionMiddleItem, pathInterface, spaceColumn,
                                        placedSpaceItems, spaceItemChain, spaceYPlacement);
                                    if (newSpaceConnection)
                                    {
                                        connectionChain.append(newSpaceConnection);
                                    }

                                    spaceItemChain.append(connectionMiddleItem);

                                    connectionStartItem = connectionMiddleItem;
                                }
                            }
                        }

                        if (!placedSpaceItems->contains(connectionStartItem) && !filterAddressSpaceChains_)
                        {
                            baseAddressNumber = pathInterface->getBaseAddress().toULongLong();
                        }
                        else
                        {
                            mirroredSlaveAddressChange += pathInterface->getBaseAddress().toULongLong();
                        }
                    }
                }
            }

            if (!connectionStartItem->isVisible())
            {
                connectionStartItem->setVisible(true);
            }

            bool spaceItemPlaced = false;
            if (!placedSpaceItems->contains(connectionStartItem))
            {
                placedSpaceItems->append(connectionStartItem);
                spaceItemPlaced = true;
            }

            quint64 memoryMapBaseAddress = connectionEndItem->getBaseAddress();

            if (!hasRemapRange)
            {
                memoryMapEndAddress = connectionEndItem->getLastAddress();
            }

            quint64 endAddressNumber = baseAddressNumber + memoryMapEndAddress;

            quint64 remappedAddress = baseAddressNumber + mirroredSlaveAddressChange;
            QString startAddress = QString::number(remappedAddress);
            QString endAddress = QString::number(endAddressNumber + mirroredSlaveAddressChange);

            quint64 yTransfer = (memoryMapBaseAddress + mirroredSlaveAddressChange) *
                MemoryDesignerConstants::RANGEINTERVAL;

            QPointF startConnectionPosBefore = connectionStartItem->pos();

            if (!placedMapItems->contains(connectionEndItem))
            {
                connectionEndItem->hideMemoryRangeLabels();
                connectionEndItem->setPos(connectionEndItem->pos().x(), connectionStartItem->pos().y() + yTransfer);

                QPointF startItemPositionBefore = connectionStartItem->pos();

                if (spaceItemPlaced)
                {
                    repositionMemoryMap(placedMapItems, placedSpaceItems, connectionStartItem, spaceColumn,
                        connectionEndItem, yTransfer);
                }
                else
                {
                    checkMemoryMapRepositionToOverlapColumn(placedMapItems, connectionEndItem, memoryMapColumn,
                        startAddress, endAddress, connectionStartItem);
                }

                QPointF startItemPositionAfter = connectionStartItem->pos();

                spaceYPlacement += startItemPositionAfter.y() - startItemPositionBefore.y();

                placedMapItems->append(connectionEndItem);
            }
            else
            {
                placeSpaceItemToOtherColumn(connectionStartItem, spaceColumn, connectionEndItem, yTransfer);

                spaceYPlacement = spaceYPlacement - (connectionStartItem->boundingRect().height() +
                    MemoryDesignerConstants::SPACEITEMINTERVAL);
            }

            if (spaceItemPlaced)
            {
                connectionStartItem->changeChildItemRanges(baseAddressNumber);
            }
            connectionEndItem->changeChildItemRanges(remappedAddress);

            qreal spaceItemEndPointBefore = connectionStartItem->getSceneEndPoint();

            MemoryConnectionItem* newConnectionItem = new MemoryConnectionItem(connectionStartItem, startAddress,
                endAddress, connectionEndItem, spaceColumn->scene(), condenseMemoryItems_, yTransfer);
            connectionChain.append(newConnectionItem);
            setHeightForConnectionChain(connectionChain);
            connectionsToMemoryMaps_.append(newConnectionItem);

            qreal spaceItemEndPointAfter = connectionStartItem->getSceneEndPoint();

            spaceYPlacement += spaceItemEndPointAfter - spaceItemEndPointBefore;
        }
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::getMainGraphicsItem()
//-----------------------------------------------------------------------------
MainMemoryGraphicsItem* MemoryDesignerDiagram::getMainGraphicsItem(
    QSharedPointer<ConnectivityInterface> connectionInterface, QString columnType) const
{
    MainMemoryGraphicsItem* graphicsItem = 0;

    QSharedPointer<MemoryItem> memoryItem = connectionInterface->getConnectedMemory();
    QSharedPointer<ConnectivityComponent> connectionInstance = connectionInterface->getInstance();
    if (memoryItem && connectionInstance)
    {
        QVector<MemoryColumn*> matchingColumns = getSpecifiedColumns(columnType);
        foreach (MemoryColumn* currentColumn, matchingColumns)
        {
            graphicsItem = currentColumn->findGraphicsItemByMemoryItem(memoryItem);
            if (graphicsItem)
            {
                break;
            }
        }
    }

    return graphicsItem;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createSpaceConnection()
//-----------------------------------------------------------------------------
MemoryConnectionItem* MemoryDesignerDiagram::createSpaceConnection(MainMemoryGraphicsItem* connectionStartItem,
    MainMemoryGraphicsItem* connectionMiddleItem, QSharedPointer<ConnectivityInterface> newSpaceInterface,
    MemoryColumn* spaceColumn, QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedSpaceItems,
    QVector<MainMemoryGraphicsItem*> spaceItemChain, int& spaceYPlacement)
{
    MemoryConnectionItem* addressSpaceConnection = 0;

    if (connectionMiddleItem)
    {
        connectionMiddleItem->hideMemoryRangeLabels();

        int startItemPositionY = connectionStartItem->pos().y();
        if (!placedSpaceItems->contains(connectionMiddleItem))
        {
            moveAddressSpaceItem(connectionMiddleItem, spaceColumn, startItemPositionY);
            connectionMiddleItem->hideFirstAndLastSegmentRange();
        }

        bool recalculateYPosition = false;
        if (!placedSpaceItems->contains(connectionStartItem))
        {
            quint64 middleItemY = connectionMiddleItem->pos().y();

            changeMasterAddressSpaceColumn(connectionStartItem, middleItemY, spaceColumn, spaceItemChain);

            placedSpaceItems->append(connectionStartItem);
            recalculateYPosition = true;
        }

        quint64 interfaceBaseAddress = newSpaceInterface->getBaseAddress().toULongLong();
        quint64 startItemBaseAddress = connectionStartItem->getBaseAddress();

        quint64 middleItemBaseAddress = connectionMiddleItem->getBaseAddress();
        quint64 middleItemLastAddress = connectionMiddleItem->getLastAddress();
        QString middleItemRangeStart = QString::number(middleItemBaseAddress + interfaceBaseAddress);
        QString middleItemRangeEnd = QString::number(middleItemLastAddress + interfaceBaseAddress);

        quint64 baseAddressDifference = interfaceBaseAddress - startItemBaseAddress;
        int yTransfer = baseAddressDifference * MemoryDesignerConstants::RANGEINTERVAL;

        if (!placedSpaceItems->contains(connectionMiddleItem))
        {
            connectionMiddleItem->setPos(
                connectionMiddleItem->pos().x(), connectionMiddleItem->pos().y() + yTransfer);
        }

        if (recalculateYPosition)
        {
            quint64 newYPosition =
                connectionMiddleItem->sceneBoundingRect().height() + MemoryDesignerConstants::SPACEITEMINTERVAL;
            if (newYPosition > spaceYPlacement)
            {
                spaceYPlacement = newYPosition;
            }
        }

        if (!placedSpaceItems->contains(connectionMiddleItem) || !placedSpaceItems->contains(connectionStartItem))
        {
            addressSpaceConnection = new MemoryConnectionItem(connectionStartItem, middleItemRangeStart,
                middleItemRangeEnd, connectionMiddleItem, spaceColumn->scene(), condenseMemoryItems_, yTransfer);
        }
        else
        {
            addressSpaceConnection = getAddressSpaceChainConnection(connectionStartItem, connectionMiddleItem);
        }
    }

    return addressSpaceConnection;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::changeMasterAddressSpaceColumn()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::changeMasterAddressSpaceColumn(MainMemoryGraphicsItem* masterSpaceItem,
    qreal spaceItemY, GraphicsColumn* originalColumn, QVector<MainMemoryGraphicsItem*> spaceItemChain)
{
    originalColumn->removeItem(masterSpaceItem);

    qreal columnWidth = originalColumn->boundingRect().width();
    qreal currentColumnPosition = originalColumn->pos().x() - columnWidth;
    GraphicsColumn* currentColumn = 0;
    while (currentColumnPosition >= 0)
    {
        currentColumn = layout_->findColumnAt(QPointF(currentColumnPosition, 0));
        if (currentColumn &&
            currentColumn->name().contains(MemoryDesignerConstants::ADDRESSSPACECOLUMN_NAME, Qt::CaseInsensitive))
        {
            currentColumn->addItem(masterSpaceItem);
            masterSpaceItem->setPos(masterSpaceItem->pos().x(), spaceItemY);

            if (!spaceItemCollidesWithOtherSpaceItems(masterSpaceItem))
            {
                return;
            }
            else
            {
                changeCollidingMasterAddressSpaceColumn(currentColumn, spaceItemChain);

                if (!spaceItemCollidesWithOtherSpaceItems(masterSpaceItem))
                {
                    return;
                }
                else
                {
                    currentColumn->removeItem(masterSpaceItem);
                }
            }
        }

        currentColumnPosition -= columnWidth;
    }

    MemoryColumn* newSpaceColumn = createAddressSpaceColumn();
    newSpaceColumn->setZValue(-1);
    newSpaceColumn->addItem(masterSpaceItem);
    masterSpaceItem->setPos(masterSpaceItem->pos().x(), spaceItemY);
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::spaceItemCollidesWithOtherSpaceItems()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::spaceItemCollidesWithOtherSpaceItems(MainMemoryGraphicsItem* spaceItem) const
{
    foreach (QGraphicsItem* collidingItem, spaceItem->collidingItems(Qt::IntersectsItemShape))
    {
        AddressSpaceGraphicsItem* collidingSpaceItem = dynamic_cast<AddressSpaceGraphicsItem*>(collidingItem);
        if (collidingSpaceItem)
        {
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::changeCollidingMasterAddressSpaceColumn()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::changeCollidingMasterAddressSpaceColumn(GraphicsColumn* currentColumn,
    QVector<MainMemoryGraphicsItem*> spaceItemChain)
{
    foreach (MainMemoryGraphicsItem* currentSpaceChainItem, spaceItemChain)
    {
        GraphicsColumn* chainParent =
            dynamic_cast<GraphicsColumn*>(currentSpaceChainItem->parentItem());
        if (chainParent && chainParent == currentColumn)
        {
            changeMasterAddressSpaceColumn(
                currentSpaceChainItem, currentSpaceChainItem->pos().y(), currentColumn, spaceItemChain);
            return;
        }
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::getAddressSpaceChainConnection()
//-----------------------------------------------------------------------------
MemoryConnectionItem* MemoryDesignerDiagram::getAddressSpaceChainConnection(
    MainMemoryGraphicsItem* connectionStartItem, MainMemoryGraphicsItem* connectionMiddleItem) const
{
    QMapIterator<quint64, MemoryConnectionItem*> connectionIterator(connectionStartItem->getMemoryConnections());
    while (connectionIterator.hasNext())
    {
        connectionIterator.next();

        MemoryConnectionItem* connectionItem = connectionIterator.value();
        if (connectionMiddleItem->getConnectionsInVector().contains(connectionItem))
        {
            return connectionItem;
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::setHeightForConnectionChain()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::setHeightForConnectionChain(QVector<MemoryConnectionItem*> connectionChain)
{
    foreach (MemoryConnectionItem* connectionItem, connectionChain)
    {
        connectionItem->setConnectionWidth();
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::checkMemoryMapRepositionToOverlapColumn()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::checkMemoryMapRepositionToOverlapColumn(
    QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedMaps, MainMemoryGraphicsItem* memoryItem,
    MemoryColumn* originalColumn, QString const& startAddress, QString const& endAddress,
    MainMemoryGraphicsItem* connectionStartItem)
{
    QRectF selectedItemRect = memoryItem->sceneBoundingRect();

    quint64 mapBaseAddress = startAddress.toULongLong();
    quint64 mapLastAddress = endAddress.toULongLong();

    int selectedItemPenWidth = memoryItem->pen().width();

    QVector<MainMemoryGraphicsItem*> connectedSpaceItems;
    connectedSpaceItems.append(connectionStartItem);

    if (memoryMapOverlapsInColumn(originalColumn, memoryItem, mapBaseAddress, mapLastAddress, selectedItemRect,
        selectedItemPenWidth, placedMaps, connectedSpaceItems))
    {
        foreach (GraphicsColumn* column, layout_->getColumns())
        {
            if (column->name().contains(QLatin1String("Overlap"), Qt::CaseInsensitive))
            {
                MemoryColumn* memoryColumn = dynamic_cast<MemoryColumn*>(column);
                if (memoryColumn)
                {
                    selectedItemRect.setX(selectedItemRect.x() + memoryColumn->boundingRect().width());

                    if (!memoryMapOverlapsInColumn(memoryColumn, memoryItem, mapBaseAddress, mapLastAddress,
                        selectedItemRect, selectedItemPenWidth, placedMaps, connectedSpaceItems))
                    {
                        originalColumn->removeItem(memoryItem);
                        memoryColumn->addItem(memoryItem);
                        return;
                    }
                }
            }
        }

        MemoryColumn* overlapColumn = createMemoryOverlapColumn();
        originalColumn->removeItem(memoryItem);
        overlapColumn->addItem(memoryItem);
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::memoryMapOverlapsInColumn()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::memoryMapOverlapsInColumn(MemoryColumn* memoryColumn,
    MainMemoryGraphicsItem* memoryItem, quint64 mapBaseAddress, quint64 mapLastAddress, QRectF memoryItemRect,
    int memoryPenWidth, QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedMaps,
    QVector<MainMemoryGraphicsItem*> connectedSpaceItems) const
{
    foreach (QGraphicsItem* graphicsItem, memoryColumn->childItems())
    {
        MemoryMapGraphicsItem* comparisonMemoryItem = dynamic_cast<MemoryMapGraphicsItem*>(graphicsItem);
        if (comparisonMemoryItem && comparisonMemoryItem != memoryItem &&
            comparisonMemoryItem->isConnectedToSpaceItems(connectedSpaceItems) &&
            placedMaps->contains(comparisonMemoryItem) &&
            memoryMapOverlapsAnotherMemoryMap(
                mapBaseAddress, mapLastAddress, memoryItemRect, memoryPenWidth, comparisonMemoryItem))
        {
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::memoryMapOverlapsAnotherMemoryMap()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::memoryMapOverlapsAnotherMemoryMap(quint64 mapBaseAddress, quint64 mapLastAddress,
    QRectF selectedMapRect, int selectedMapPenWidth, MainMemoryGraphicsItem* comparisonMemoryItem) const
{
    QRectF comparisonRectangle = comparisonMemoryItem->sceneBoundingRect();

    if (selectedMapRect.x() == comparisonRectangle.x())
    {
        int mapItemLineWidth = comparisonMemoryItem->pen().width();

        qreal comparisonItemEnd = comparisonRectangle.bottom();

        foreach (MemoryConnectionItem* connectionItem, comparisonMemoryItem->getMemoryConnections())
        {
            qreal connectionLow = connectionItem->sceneBoundingRect().bottom();
            if (connectionLow > comparisonItemEnd)
            {
                comparisonItemEnd = connectionLow;
            }
        }

        comparisonRectangle.setBottom(comparisonItemEnd);

        quint64 comparisonBaseAddress = comparisonMemoryItem->getBaseAddress();
        quint64 comparisonLastAddress = comparisonMemoryItem->getLastAddress();
        MemoryConnectionItem* comparisonItemLastConnection = comparisonMemoryItem->getLastConnection();
        if (comparisonItemLastConnection)
        {
            comparisonBaseAddress += comparisonItemLastConnection->getRangeStartValue().toULongLong(0, 16);
            comparisonLastAddress = comparisonItemLastConnection->getRangeEndValue().toULongLong(0, 16);
        }
        if (itemCollidesWithAnotherItem(
            selectedMapRect, selectedMapPenWidth, comparisonRectangle, mapItemLineWidth) ||
            (mapBaseAddress >= comparisonBaseAddress && mapBaseAddress <= comparisonLastAddress) ||
            (comparisonBaseAddress >= mapBaseAddress && comparisonBaseAddress <= mapLastAddress))
        {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::repositionMemoryMap()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::repositionMemoryMap(QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedMapItems,
    QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedSpaceItems,
    MainMemoryGraphicsItem* startConnectionItem, MemoryColumn* addressSpaceColumn,
    MainMemoryGraphicsItem* endConnectionItem, int memoryMapYTransfer)
{
    foreach (MemoryDesignerGraphicsItem* memoryMapItem, *placedMapItems)
    {
        QRectF endConnectionRectangle = endConnectionItem->sceneBoundingRect();
        int endConnectionPenWidth = endConnectionItem->pen().width();
        QRectF comparisonRectangle = memoryMapItem->sceneBoundingRect();
        int comparisonPenWidth = memoryMapItem->pen().width();

        if (endConnectionItem != memoryMapItem && itemCollidesWithAnotherItem(
            endConnectionRectangle, endConnectionPenWidth, comparisonRectangle, comparisonPenWidth))
        {
            QPointF collidingMapItemLowLeft =
                memoryMapItem->mapToScene(memoryMapItem->boundingRect().bottomLeft());
            int mapItemYTransfer = collidingMapItemLowLeft.y() + GridSize;

            endConnectionItem->setPos(endConnectionItem->x(), mapItemYTransfer);

            startConnectionItem->setPos(
                startConnectionItem->pos().x(), endConnectionItem->pos().y() - memoryMapYTransfer);

            repositionSpaceItemToMemoryMap(
                placedSpaceItems, startConnectionItem, endConnectionItem, addressSpaceColumn, memoryMapYTransfer);

            addressSpaceColumn->onMoveItem(startConnectionItem);
            addressSpaceColumn->onReleaseItem(startConnectionItem);

            repositionMemoryMap(placedMapItems, placedSpaceItems, startConnectionItem, addressSpaceColumn,
                endConnectionItem, memoryMapYTransfer);

            return;
        }
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::repositionSpaceItemToMemoryMap()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::repositionSpaceItemToMemoryMap(
    QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedSpaceItems, MainMemoryGraphicsItem* startItem,
    MainMemoryGraphicsItem* endItem, MemoryColumn* spaceColumn, int memoryMapYTransfer)
{
    foreach (MemoryDesignerGraphicsItem* spaceItem, *placedSpaceItems)
    {
        if (spaceItem != startItem && startItem->collidesWithItem(spaceItem))
        {
            QPointF collidingSpaceItemLowLeft =
                spaceItem->mapToScene(spaceItem->boundingRect().bottomLeft());
            int spaceItemYTransfer = collidingSpaceItemLowLeft.y() + MemoryDesignerConstants::SPACEITEMINTERVAL;

            startItem->setPos(startItem->x(), spaceItemYTransfer);

            endItem->setPos(endItem->x(), startItem->y() + memoryMapYTransfer);

            repositionSpaceItemToMemoryMap(placedSpaceItems, startItem, endItem, spaceColumn, memoryMapYTransfer);

            return;
        }
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::placeSpaceItemToOtherColumn()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::placeSpaceItemToOtherColumn(MainMemoryGraphicsItem* spaceItem,
    MemoryColumn* originalColumn, MainMemoryGraphicsItem* targetItem, int yTransfer)
{
    originalColumn->removeItem(spaceItem);

    foreach (GraphicsColumn* column, layout_->getColumns())
    {
        MemoryColumn* currentSpaceColumn = dynamic_cast<MemoryColumn*>(column);
        if (currentSpaceColumn && currentSpaceColumn->name().contains(
            MemoryDesignerConstants::ADDRESSSPACECOLUMN_NAME, Qt::CaseInsensitive))
        {
            currentSpaceColumn->addItem(spaceItem);
            spaceItem->setPos(spaceItem->pos().x(), targetItem->pos().y() + yTransfer);

            if (spaceItemCollidesWithOtherSpaceItems(spaceItem))
            {
                currentSpaceColumn->removeItem(spaceItem);
            }
            else
            {
                return;
            }
        }
    }

    MemoryColumn* newSpaceColumn = createAddressSpaceColumn();
    newSpaceColumn->addItem(spaceItem);
    spaceItem->setPos(spaceItem->pos().x(), targetItem->pos().y());
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::reDrawConnections()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::reDrawConnections(QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedSpaceItems)
{
    foreach (MainMemoryGraphicsItem* spaceItem, *placedSpaceItems)
    {
        spaceItem->reDrawConnections();
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::moveUnconnectedAddressSpaces()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::moveUnconnectedAddressSpaces(
    QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedSpaceItems, int& spaceYPlacement,
    MemoryColumn* spaceColumn)
{
    spaceYPlacement += GridSize * 10;

    foreach (QGraphicsItem* graphicsItem, spaceColumn->getItems())
    {
        MainMemoryGraphicsItem* memoryGraphicsItem = dynamic_cast<MainMemoryGraphicsItem*>(graphicsItem);
        if (memoryGraphicsItem && !placedSpaceItems->contains(memoryGraphicsItem))
        {
            moveAddressSpaceItem(memoryGraphicsItem, spaceColumn, spaceYPlacement);
        }
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::moveAddressSpaceItem()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::moveAddressSpaceItem(MainMemoryGraphicsItem* spaceItem, MemoryColumn* spaceColumn,
    int& spaceYPlacement)
{
    spaceItem->setPos(spaceItem->x(), spaceYPlacement);
    spaceColumn->onMoveItem(spaceItem);
    spaceColumn->onReleaseItem(spaceItem);

    spaceYPlacement += spaceItem->boundingRect().height() + MemoryDesignerConstants::SPACEITEMINTERVAL;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::moveUnconnectedMemoryMaps()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::moveUnconnectedMemoryMaps(
    QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedMapItems, MemoryColumn* memoryMapColumn)
{
    if (!placedMapItems->isEmpty())
    {
        qreal lastMapItemBottomLinePositionY = 0;
        foreach (MainMemoryGraphicsItem* memoryMapItem, *placedMapItems)
        {
            int extensionHeight = 0;
            MemoryExtensionGraphicsItem* extensionItem = memoryMapItem->getExtensionItem();
            if (extensionItem)
            {
                extensionHeight = extensionItem->sceneBoundingRect().height();
            }

            qreal mapItemBottom = memoryMapItem->sceneBoundingRect().bottom() + extensionHeight;
            if (mapItemBottom > lastMapItemBottomLinePositionY)
            {
                lastMapItemBottomLinePositionY = mapItemBottom;
            }
        }

        int separationY = GridSize * 10 + lastMapItemBottomLinePositionY;

            foreach (QGraphicsItem* graphicsItem, memoryMapColumn->getItems())
            {
                MainMemoryGraphicsItem* memoryGraphicsItem = dynamic_cast<MainMemoryGraphicsItem*>(graphicsItem);
                if (memoryGraphicsItem && !placedMapItems->contains(memoryGraphicsItem))
                {
                    memoryGraphicsItem->setPos(memoryGraphicsItem->x(), separationY);

                    separationY += memoryGraphicsItem->boundingRect().height() + GridSize;
                }
            }
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::findColumnByName()
//-----------------------------------------------------------------------------
MemoryColumn* MemoryDesignerDiagram::findColumnByName(QString const& columnName) const
{
    foreach (GraphicsColumn* column, layout_->getColumns())
    {
        MemoryColumn* memoryColumn = dynamic_cast<MemoryColumn*>(column);
        if (memoryColumn)
        {
            QSharedPointer<ColumnDesc> memoryDesc = memoryColumn->getColumnDesc();
            if (memoryDesc && memoryDesc->name().compare(columnName) == 0)
            {
                return memoryColumn;
            }
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::extendMemoryItem()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::extendMemoryItem(MainMemoryGraphicsItem* memoryGraphicsItem,
    MemoryConnectionItem* connectionItem, int& spaceYplacement)
{
    QPointF spaceTopLeft = memoryGraphicsItem->boundingRect().topLeft();
    QPointF spaceLowRight = memoryGraphicsItem->boundingRect().bottomRight();

    QPointF connectionTopLeft =
        memoryGraphicsItem->mapFromItem(connectionItem, connectionItem->boundingRect().topLeft());
    QPointF connectionLowRight =
        memoryGraphicsItem->mapFromItem(connectionItem, connectionItem->boundingRect().bottomRight());

    if (connectionLowRight.y() > spaceLowRight.y())
    {
        qreal positionX = spaceTopLeft.x() + 0.5;
        qreal extensionWidth = spaceLowRight.x() - spaceTopLeft.x() - 1;
        qreal positionY = spaceLowRight.y() - 0.5;
        qreal extensionHeight = connectionLowRight.y() - spaceLowRight.y();

        MemoryExtensionGraphicsItem* extensionItem = new MemoryExtensionGraphicsItem(
            positionX, positionY, extensionWidth, extensionHeight, memoryGraphicsItem);
        memoryGraphicsItem->setExtensionItem(extensionItem);

        spaceYplacement += extensionHeight;
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::createOverlappingConnectionMarkers()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::createOverlappingConnectionMarkers(
    QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedSpaceItems)
{
    foreach (MainMemoryGraphicsItem* spaceItem, *placedSpaceItems)
    {
        if (spaceItem->getMemoryConnections().size() > 1)
        {
            QVector<MemoryConnectionItem*> spaceConnections = spaceItem->getConnectionsInVector();

            for (int selectedIndex = 0; selectedIndex < spaceItem->getMemoryConnections().size() - 1;
                ++selectedIndex)
            {
                for (int comparisonIndex = selectedIndex + 1;
                    comparisonIndex < spaceItem->getMemoryConnections().size(); ++comparisonIndex)
                {
                    MemoryConnectionItem* selectedItem = spaceConnections.at(selectedIndex);
                    MemoryConnectionItem* comparisonItem = spaceConnections.at(comparisonIndex);

                    QRectF connectionRect = selectedItem->sceneBoundingRect();
                    QRectF comparisonRect = comparisonItem->sceneBoundingRect();

                    if (selectedItem && comparisonItem && selectedItem != comparisonItem &&
                        selectedItem->endItemIsMemoryMap() && comparisonItem->endItemIsMemoryMap() &&
                        itemCollidesWithAnotherItem(connectionRect, selectedItem->pen().width(), comparisonRect,
                        comparisonItem->pen().width()))
                    {
                        MemoryCollisionItem* newCollisionItem =
                            new MemoryCollisionItem(selectedItem, comparisonItem, spaceItem->scene());
                        spaceItem->addConnectionCollision(newCollisionItem);
                        memoryCollisions_.append(newCollisionItem);
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::itemCollidesWithAnotherItem()
//-----------------------------------------------------------------------------
bool MemoryDesignerDiagram::itemCollidesWithAnotherItem(QRectF firstRectangle, int firstLineWidth,
    QRectF secondRectangle, int secondLineWidth) const
{
    qreal firstItemTop = firstRectangle.topLeft().y() + firstLineWidth;
    qreal firstItemLow = firstRectangle.bottomLeft().y() - firstLineWidth;
    qreal secondItemTop = secondRectangle.topLeft().y() + secondLineWidth;
    qreal secondItemLow = secondRectangle.bottomLeft().y() - secondLineWidth;

    if (
        ((firstItemTop > secondItemTop && firstItemTop < secondItemLow) ||
        (firstItemLow < secondItemLow && firstItemLow > secondItemTop) ||

        (secondItemTop > firstItemTop && secondItemTop < firstItemLow) ||
        (secondItemLow < firstItemLow && secondItemLow > firstItemTop) ||

        (firstItemTop == secondItemTop && firstItemLow == secondItemLow)))
    {
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::compressGraphicsItems()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::compressGraphicsItems(
    QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedSpaceItems, int& spaceYPlacement,
    MemoryColumn* spaceColumn)
{
    QSharedPointer<QVector<MemoryConnectionItem*> > movedConnectionItems (new QVector<MemoryConnectionItem*>());

    foreach (GraphicsColumn* graphicsColumn, layout_->getColumns())
    {
        MemoryColumn* memoryColumn = dynamic_cast<MemoryColumn*>(graphicsColumn);
        if (memoryColumn)
        {
            int yTransfer = 0;
            quint64 spaceItemLowAfter = 0;

            foreach (QGraphicsItem* graphicsItem, memoryColumn->getGraphicsItemInOrder())
            {
                MainMemoryGraphicsItem* memoryItem = dynamic_cast<MainMemoryGraphicsItem*>(graphicsItem);
                if (memoryItem)
                {
                    int memoryItemLowBefore = memoryItem->getSceneEndPoint();

                    if (condenseMemoryItems_)
                    {
                        memoryItem->condenseItemAndChildItems(movedConnectionItems);
                    }

                    MemoryConnectionItem* lastConnection = memoryItem->getLastConnection();
                    if (lastConnection)
                    {
                        extendMemoryItem(memoryItem, lastConnection, spaceYPlacement);
                    }

                    if (condenseMemoryItems_)
                    {
                        AddressSpaceGraphicsItem* spaceItem = dynamic_cast<AddressSpaceGraphicsItem*>(memoryItem);
                        if (spaceItem && placedSpaceItems->contains(memoryItem) && memoryColumn == spaceColumn)
                        {
                            quint64 spaceItemTop = spaceItem->sceneBoundingRect().top();

                            if (spaceItemTop + spaceItem->pen().width() != spaceItemLowAfter)
                            {
                                qint64 spaceInterval = (spaceItemTop + yTransfer) - spaceItemLowAfter;

                                if (spaceInterval < MemoryDesignerConstants::SPACEITEMINTERVAL)
                                {
                                    quint64 yMovementAddition = MemoryDesignerConstants::SPACEITEMINTERVAL -
                                        ((spaceItemTop + yTransfer) - spaceItemLowAfter);
                                    yTransfer += yMovementAddition;
                                }

                                spaceItem->moveConnectedItems(yTransfer);

                            }

                            spaceItemLowAfter = spaceItem->getSceneEndPoint();

                            yTransfer = spaceItemLowAfter - memoryItemLowBefore;

                            spaceYPlacement = spaceItemLowAfter + MemoryDesignerConstants::SPACEITEMINTERVAL;
                        }

                        memoryItem->resizeSubItemNameLabels();
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::repositionCompressedMemoryMaps()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::repositionCompressedMemoryMaps(
    QSharedPointer<QVector<MainMemoryGraphicsItem*> > placedMapItems, MemoryColumn* memoryMapColumn)
{
    foreach (MainMemoryGraphicsItem* mapItem, *placedMapItems)
    {
        QGraphicsItem* mapParentItem = mapItem->parentItem();
        MemoryColumn* originalColumn = dynamic_cast<MemoryColumn*>(mapParentItem);
        if (originalColumn && originalColumn != memoryMapColumn)
        {
            quint64 mapBaseAddress = mapItem->getBaseAddress();
            quint64 mapLastAddress = mapItem->getLastAddress();

            QRectF mapRectangle = mapItem->sceneBoundingRect();
            int mapPenWidth = mapItem->pen().width();

            if (mapItem->hasExtensionItem())
            {
                mapRectangle.setHeight(mapRectangle.height() + mapItem->getExtensionItem()->boundingRect().height());
            }

            QVector<MainMemoryGraphicsItem*> connectedSpaceItems;
            foreach (MemoryConnectionItem* connectionItem, mapItem->getMemoryConnections())
            {
                MainMemoryGraphicsItem* connectionStartItem = connectionItem->getConnectionStartItem();
                if (!connectedSpaceItems.contains(connectionStartItem))
                {
                    connectedSpaceItems.append(connectionStartItem);
                }
            }

            int columnWidth = originalColumn->sceneBoundingRect().width();

            QPointF columnPoint (originalColumn->pos().x() - columnWidth, mapRectangle.y());
            MemoryColumn* comparisonColumn = dynamic_cast<MemoryColumn*>(layout_->findColumnAt(columnPoint));
            if (comparisonColumn)
            {
                while (comparisonColumn && !comparisonColumn->name().contains(
                    MemoryDesignerConstants::ADDRESSSPACECOLUMN_NAME, Qt::CaseInsensitive))
                {
                    mapRectangle.setX(mapRectangle.x() - columnWidth);

                    if (!memoryMapOverlapsInColumn(comparisonColumn, mapItem, mapBaseAddress, mapLastAddress,
                        mapRectangle, mapPenWidth, placedMapItems, connectedSpaceItems))
                    {
                        originalColumn->removeItem(mapItem);
                        comparisonColumn->addItem(mapItem, true);

                        if (originalColumn->getItems().isEmpty() && originalColumn->name().contains(
                            MemoryDesignerConstants::MEMORYMAPOVERLAPCOLUMN_NAME, Qt::CaseInsensitive))
                        {
                            layout_->removeColumn(originalColumn);
                        }

                        break;
                    }

                    columnPoint.setX(columnPoint.x() - columnWidth);
                    comparisonColumn = dynamic_cast<MemoryColumn*>(layout_->findColumnAt(columnPoint));
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::getSpecifiedColumns()
//-----------------------------------------------------------------------------
QVector<MemoryColumn*> MemoryDesignerDiagram::getSpecifiedColumns(QString const& columnSpecification) const
{
    QVector<MemoryColumn*> foundColumns;
    
    foreach (GraphicsColumn* column, layout_->getColumns())
    {
        MemoryColumn* currentColumn = dynamic_cast<MemoryColumn*>(column);
        if (currentColumn && currentColumn->name().contains(columnSpecification, Qt::CaseInsensitive))
        {
            foundColumns.append(currentColumn);
        }
    }

    return foundColumns;
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::onShow()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::onShow()
{

}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::onVerticalScroll()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::onVerticalScroll(qreal y)
{
    layout_->setOffsetY(y);
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::wheelEvent()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if (event->modifiers() == Qt::CTRL && parentDocument_)
    {
        // Set the zoom level and center the view.
        parentDocument_->setZoomLevel(parentDocument_->getZoomLevel() + event->delta() / 12);

        event->accept();
    }
    else if (event->modifiers() == Qt::ALT && !addressBlockRegistersAreFiltered() && !fieldsAreFiltered())
    {
        qreal deltaWidth = event->delta();
        deltaWidth = -deltaWidth;

        qreal previousWidthBoundary = widthBoundary_;
        widthBoundary_ += deltaWidth;
        if (widthBoundary_ < 0)
        {
            deltaWidth = previousWidthBoundary;
            widthBoundary_ = 0;
        }

        if (deltaWidth != 0)
        {
            foreach (GraphicsColumn* column, layout_->getColumns())
            {
                MemoryColumn* memoryColumn = dynamic_cast<MemoryColumn*>(column);
                if (memoryColumn && memoryColumn->containsMemoryMapItems())
                {
                    memoryColumn->changeWidth(deltaWidth);
                }
            }
            foreach (MemoryConnectionItem* connectionItem, connectionsToMemoryMaps_)
            {
                connectionItem->reDrawConnection();
            }
            foreach (MemoryCollisionItem* collisionItem, memoryCollisions_)
            {
                collisionItem->reDrawCollision();
            }
        }

        event->accept();
    }
    else
    {
        QGraphicsScene::wheelEvent(event);
    }
}

//-----------------------------------------------------------------------------
// Function: MemoryDesignerDiagram::drawBackground()
//-----------------------------------------------------------------------------
void MemoryDesignerDiagram::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->setWorldMatrixEnabled(true);
    painter->setPen(QPen(Qt::black, 0));

    qreal left = int(rect.left()) - (int(rect.left()) % GridSize );
    qreal top = int(rect.top()) - (int(rect.top()) % GridSize );

    for (qreal x = left; x < rect.right(); x += GridSize )
    {
//         for (qreal y = top; y < rect.bottom(); y += 3 * GridSize )
        for (qreal y = top; y < rect.bottom(); y += GridSize * 3)
        {
            painter->drawPoint(x, y);
        }
    }
}
