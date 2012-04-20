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
	tag root("export");

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
				tag m("mesh");

				// get the mesh vertices
				{
					tag t_vertices("vertices");

					MFloatPointArray vertices;
					status = meshFn.getPoints(vertices);
					assert(status == MStatus::kSuccess);
					mout << "    " << vertices.length() << " vertices" << endl;

					for(unsigned a=0;a<vertices.length();a++) {
						tag v("vertex");
						v.addAttribute("x", vertices[a].x);
						v.addAttribute("y", vertices[a].y);
						v.addAttribute("z", vertices[a].z);
					}
				}

				// output the polygons
				{
					tag t_polys("polygons");

					mout << "    " << meshFn.numPolygons() << " polygons" << endl;

					MIntArray vertexList;
					for(int p=0;p<meshFn.numPolygons();p++) {
						tag t_poly("polygon");

						status = meshFn.getPolygonVertices(p, vertexList);
						assert(status == MStatus::kSuccess);

						for(unsigned a=0;a<vertexList.length();a++) {
							stringstream s;
							s << "v" << a;
							t_poly.addAttribute(s.str(), vertexList[a]);
						}
					}
				}
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