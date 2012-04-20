#include "main.h"

#include <cassert>

#include <maya/MFnPlugin.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MItSelectionList.h>
#include <maya/MFn.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVectorArray.h>

#include "maya_stream.h"
#include "tag.h"

using namespace std;

exporter::exporter() {
}

void* exporter::creator() {
	return new exporter;
}

MStatus exporter::doIt(const MArgList& argList) {
	maya_stream mout;

	mout << "exporting..." << endl;
	tag::openFile("C:\\Documents and Settings\\Martin\\Desktop\\export.xml");
	tag root("root");

	{
		tag test("test");
		test.addAttribute("asfd", 10);
		test.addAttribute("qwer", "aaa");
	}

	{
		tag test2("test2");
	}

	MStatus status;

	// get active selection iterator
	MSelectionList activeList;
    MGlobal::getActiveSelectionList(activeList);

	MStringArray list;
	activeList.getSelectionStrings(list);
	mout << "  selection: ";
	for(unsigned a=0;a<list.length();a++)
		mout << list[a] << "  ";
	mout << endl;

	for(unsigned a=0;a<activeList.length();a++) {
        MDagPath item;
        MObject component;
        status = activeList.getDagPath(a, item, component);

		if(status == MStatus::kSuccess) {
			// get only meshes
			if(item.hasFn(MFn::kMesh)) {
				// get the mesh function set
				MFnMesh meshFn(item, &status);
				assert(status == MStatus::kSuccess);

				// output the mesh name
				mout << "  MESH: " << meshFn.name() << endl;

				// get the mesh vertices
				MFloatPointArray vertices;
				status = meshFn.getPoints(vertices);
				assert(status == MStatus::kSuccess);
				mout << "    " << vertices.length() << " vertices" << endl;

				// get the mesh normals
				MFloatVectorArray normals;
				status = meshFn.getNormals(normals);
				assert(status == MStatus::kSuccess);
				mout << "    " << normals.length() << " normals" << endl;

				assert(vertices.length() == normals.length());

				// output the polygons
				mout << "    " << meshFn.numPolygons() << " polygons" << endl;
			}
		}
    }

    return MS::kSuccess;
}
 
MStatus initializePlugin( MObject obj )
{
    MFnPlugin plugin( obj, "Martin Prazak", "1.0", "Any");
    MStatus status = plugin.registerCommand( "meshExport", exporter::creator );
    CHECK_MSTATUS_AND_RETURN_IT( status );
    return status;
}
 
MStatus uninitializePlugin( MObject obj )
{
    MFnPlugin plugin( obj );
    MStatus status = plugin.deregisterCommand( "meshExport" );
    CHECK_MSTATUS_AND_RETURN_IT( status );
    return status;
}