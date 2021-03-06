//-----------------------------------------------------------------------------
// File: absdefgroup.cpp
//-----------------------------------------------------------------------------
// Project: Kactus2
// Author: Antti Kamppi
// Date: 15.8.2011
//
// Description:
// Editor for the logical ports of an abstraction definition.
//-----------------------------------------------------------------------------

#include "absdefgroup.h"

#include <IPXACTmodels/BusDefinition/BusDefinition.h>

#include <editors/BusDefinitionEditor/AbstractionDefinitionPortsSortFilter.h>

#include <common/widgets/vlnvDisplayer/vlnvdisplayer.h>
#include <common/widgets/vlnvEditor/vlnveditor.h>

#include <QSortFilterProxyModel>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
// Function: AbsDefGroup::AbsDefGroup()
//-----------------------------------------------------------------------------
AbsDefGroup::AbsDefGroup(LibraryInterface* libraryHandler, QWidget *parent):
QGroupBox(tr("Signals (Abstraction Definition)"), parent),
vlnvDisplay_(new VLNVDisplayer(this, VLNV())),
extendVLNVEditor_(new VLNVEditor(VLNV::ABSTRACTIONDEFINITION, libraryHandler, this, this)),
descriptionEditor_(new QPlainTextEdit(this)),
portTabs_(this),
wirePortsEditor_(&portTabs_),
transactionalPortsEditor_(&portTabs_),
abstraction_()
{
    extendVLNVEditor_->setToolTip(QString("Extended abstraction definition is not currently supported in Kactus2"));

    vlnvDisplay_->setTitle(QStringLiteral("Abstraction definition"));
    extendVLNVEditor_->setTitle(tr("Extended abstraction definition"));

    extendVLNVEditor_->setDisabled(true);
    extendVLNVEditor_->setMandatory(false);

    portTabs_.addTab(&wirePortsEditor_, QStringLiteral("Wire ports"));
    portTabs_.addTab(&transactionalPortsEditor_, QStringLiteral("Transactional ports"));

    connect(&wirePortsEditor_, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()), Qt::UniqueConnection);
    connect(&wirePortsEditor_, SIGNAL(noticeMessage(const QString&)),
        this, SIGNAL(noticeMessage(const QString&)), Qt::UniqueConnection);
    connect(&wirePortsEditor_, SIGNAL(errorMessage(const QString&)),
        this, SIGNAL(errorMessage(const QString&)), Qt::UniqueConnection);
    connect(&wirePortsEditor_, SIGNAL(portRenamed(const QString&, const QString&)),
        this, SIGNAL(portRenamed(const QString&, const QString&)), Qt::UniqueConnection);
    connect(&wirePortsEditor_, SIGNAL(portRemoved(const QString&, const General::InterfaceMode)),
        this, SIGNAL(portRemoved(const QString&, const General::InterfaceMode)), Qt::UniqueConnection);

    connect(&transactionalPortsEditor_, SIGNAL(contentChanged()),
        this, SIGNAL(contentChanged()), Qt::UniqueConnection);
    connect(&transactionalPortsEditor_, SIGNAL(noticeMessage(const QString&)),
        this, SIGNAL(noticeMessage(const QString&)), Qt::UniqueConnection);
    connect(&transactionalPortsEditor_, SIGNAL(errorMessage(const QString&)),
        this, SIGNAL(errorMessage(const QString&)), Qt::UniqueConnection);
    connect(&transactionalPortsEditor_, SIGNAL(portRenamed(const QString&, const QString&)),
        this, SIGNAL(portRenamed(const QString&, const QString&)), Qt::UniqueConnection);
    connect(&transactionalPortsEditor_, SIGNAL(portRemoved(const QString&, const General::InterfaceMode)),
        this, SIGNAL(portRemoved(const QString&, const General::InterfaceMode)), Qt::UniqueConnection);

    connect(descriptionEditor_, SIGNAL(textChanged()), this, SLOT(onDescriptionChanged()), Qt::UniqueConnection);

	setupLayout();
}

//-----------------------------------------------------------------------------
// Function: AbsDefGroup::~AbsDefGroup()
//-----------------------------------------------------------------------------
AbsDefGroup::~AbsDefGroup()
{

}

//-----------------------------------------------------------------------------
// Function: AbsDefGroup::save()
//-----------------------------------------------------------------------------
void AbsDefGroup::save()
{
    wirePortsEditor_.save();
    transactionalPortsEditor_.save();
}

//-----------------------------------------------------------------------------
// Function: AbsDefGroup::setAbsDef()
//-----------------------------------------------------------------------------
void AbsDefGroup::setAbsDef(QSharedPointer<AbstractionDefinition> absDef)
{
    abstraction_ = absDef;

    wirePortsEditor_.setAbsDef(absDef);
    transactionalPortsEditor_.setAbsDef(absDef);
    vlnvDisplay_->setVLNV(absDef->getVlnv());

    if (abstractionContainsTransactionalPorts())
    {
        portTabs_.setCurrentWidget(&transactionalPortsEditor_);
    }

    if (absDef->getExtends().isValid())
    {
        extendVLNVEditor_->setVLNV(absDef->getExtends());
    }

    if (!absDef->getDescription().isEmpty())
    {
        descriptionEditor_->setPlainText(absDef->getDescription());
    }
}

//-----------------------------------------------------------------------------
// Function: absdefgroup::abstractionContainsTransactionalPorts()
//-----------------------------------------------------------------------------
bool AbsDefGroup::abstractionContainsTransactionalPorts() const
{
    foreach(QSharedPointer<PortAbstraction> logicalPort, *abstraction_->getLogicalPorts())
    {
        if (logicalPort->hasTransactional())
        {
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// Function: AbsDefGroup::setBusDef()
//-----------------------------------------------------------------------------
void AbsDefGroup::setBusDef(QSharedPointer<BusDefinition> busDefinition)
{
    wirePortsEditor_.setBusDef(busDefinition);
    transactionalPortsEditor_.setBusDef(busDefinition);
}

//-----------------------------------------------------------------------------
// Function: AbsDefGroup::setupLayout()
//-----------------------------------------------------------------------------
void AbsDefGroup::setupLayout()
{
    QGroupBox* descriptionGroup = new QGroupBox(tr("Description"), this);

    QVBoxLayout* descriptionLayout = new QVBoxLayout(descriptionGroup);
    descriptionLayout->addWidget(descriptionEditor_);

    QGridLayout* topLayout = new QGridLayout(this);
    topLayout->addWidget(vlnvDisplay_, 0, 0, 1, 1);
    topLayout->addWidget(extendVLNVEditor_, 0, 1, 1, 1);
    topLayout->addWidget(descriptionGroup, 0, 2, 1, 1);
    topLayout->addWidget(&portTabs_, 1, 0, 1, 3);

    topLayout->setColumnStretch(0, 25);
    topLayout->setColumnStretch(1, 25);
    topLayout->setColumnStretch(2, 50);

    topLayout->setRowStretch(0, 1);
    topLayout->setRowStretch(1, 10);
}

//-----------------------------------------------------------------------------
// Function: absdefgroup::onDescriptionChanged()
//-----------------------------------------------------------------------------
void AbsDefGroup::onDescriptionChanged()
{
    abstraction_->setDescription(descriptionEditor_->toPlainText());
    emit contentChanged();
}
