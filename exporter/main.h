#pragma once

#include <iostream>

#include <maya/MArgList.h>
#include <maya/MObject.h>
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>
 
class exporter : public MPxCommand {
	public:
		exporter();
		virtual MStatus doIt(const MArgList& argList);
		static void* creator();
};