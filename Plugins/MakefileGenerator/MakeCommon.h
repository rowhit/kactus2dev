//-----------------------------------------------------------------------------
// File: MakeCommon.h
//-----------------------------------------------------------------------------
// Project: Kactus 2
// Author: Janne Virtanen
// Date: 19.05.2016
//
// Description:
// Contains the data types and constants commonly used in the makefile generation.
//-----------------------------------------------------------------------------

#ifndef MAKECOMMON_H
#define MAKECOMMON_H

#include <QString>
#include <QSharedPointer>

#include <IPXACTmodels/Component/File.h>
#include <IPXACTmodels/Component/FileSet.h>
#include <IPXACTmodels/Component/Component.h>
#include <IPXACTmodels/Component/BuildCommand.h>

#include <IPXACTmodels/kactusExtensions/SWView.h>

// A struct containing needed data of a single file parsed to be used in a makefile.
// Provided that it is not a header file, it likely yields an object file.
struct MakeObjectData
{
	// The file which data is parsed here.
	QSharedPointer<File> file;
	// The build command directly associated with the file.
	QSharedPointer<BuildCommand> fileBuildCmd;
	// The build command of the file set where file belongs to.
	QSharedPointer<FileBuilder> fileSetBuildCmd;
	// The build command of the active software view of the software instance.
	QSharedPointer<SWFileBuilder> swBuildCmd;
	// The resolved compiler for the object file.
	QString compiler;
	// The resolved flags for the object file.
	QString flags;
	// The absolute path of the file.
	QString path;
	// The name of the file.
	QString fileName;
	// The software instance where the file is included.
	QString instanceName;
	// File set where the file is included.
	QSharedPointer<FileSet> fileSet;
};

// A struct containing parsed data of software instance in a stack.
struct StackPart
{
	// The software instance.
	QSharedPointer<SWInstance> instance;
	// The instantiated component.
	QSharedPointer<Component> component;
	// The active view in the current design configuration.
	QSharedPointer<SWView> view;
	// The selected build command of the active software view of the software instance.
	QSharedPointer<SWFileBuilder> swBuildCmd;
};

// A struct containing needed data of a single makefile.
struct MakeFileData
{
	// The parsed software stack.
	QList<QSharedPointer<StackPart> > parts_;

	// The hardware instance, where the top component of the stack is mapped to.
	QSharedPointer<ComponentInstance> hardInstance;
	// The instantiated hardware component.
	QSharedPointer<Component> hardComponent;
	// The active software view of the hardware in the current design configuration.
	QSharedPointer<SWView> hardView;

	// Parsed files found in software views of software components.
	QList<QSharedPointer<MakeObjectData> > swObjects;
	// The selected build command of the active software view of the hardware instance.
	QSharedPointer<SWFileBuilder> hwBuildCmd;
	// Flags passed down from software views.
	QStringList softViewFlags;
	// The name of the instance.
	QString name;
	// The list of all included directories.
	QStringList includeDirectories;
	// The list of all include files.
	QList<QSharedPointer<MakeObjectData> > includeFiles;
	// The list of parsed software instances. Tracked to avoid re-parsing a dependency.
	QList<QSharedPointer<SWInstance> > parsedInstances;
	// Header files associated with the component instance.
	// This is to contain the makefile generated for the instance.
	QSharedPointer<FileSet> instanceHeaders;
	// The path where the makefile is generated to.
	QString targetPath;
};

#endif // MAKECOMMON_H