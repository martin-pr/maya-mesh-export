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
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MDagPathArray.h>
#include <maya/MItGeometry.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTransform.h>
#include <maya/MQuaternion.h>
#include <maya/MAnimControl.h>

#include "maya_stream.h"
#include "tag.h"

using namespace std;

// put the mesh to binding pose
//   according to http://nccastaff.bournemouth.ac.uk/jmacey/RobTheBloke/www/research/maya/mfnskinning.htm
//   and http://svn6.assembla.com/svn/p400roguelike/P400Roguelike/OgreSDK/Tools/MayaExport/src/skeleton.cpp
//   DOES IT WORK?!
void bindingPose(const bool& set) {
	static bool isInBindingPose = false;
	MStatus status;

	// set the binding pose
	if(set) {
		if(!isInBindingPose) {
			isInBindingPose = true;

			// disable IK
			status = MGlobal::executeCommand("doEnableNodeItems false all");
			assert(status == MStatus::kSuccess);
			// save the current pose
			status = MGlobal::executeCommand("dagPose -save -name savedPose");
			assert(status == MStatus::kSuccess);
			// and move the mesh to the binding pose
			status = MGlobal::executeCommand("dagPose -restore -global -bindPose");
			assert(status == MStatus::kSuccess);
		}

	}
	// undo the binding pose
	else if(isInBindingPose) {
		isInBindingPose = false;

		// restore the saved pose
		status = MGlobal::executeCommand("dagPose -restore -global -name savedPose");
		assert(status == MStatus::kSuccess);
		
		// turn on the IK back
		status = MGlobal::executeCommand("doEnableNodeItems true all");
		assert(status == MStatus::kSuccess);
	}
}

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
        MDagPath mesh;
        MObject component;
        status = activeList.getDagPath(a, mesh, component);

		if(status == MStatus::kSuccess) {
			// get only meshes
			if(mesh.hasFn(MFn::kMesh)) {
				// get the mesh function set
				MFnMesh meshFn(mesh, &status);
				assert(status == MStatus::kSuccess);

				//////////////////////////////////
				// output the mesh name
				mout << "  Mesh: " << meshFn.name() << endl;
				tag m("mesh");

				// set the mesh to the binding pose
				bindingPose(true);

				//////////////////////////////////
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

				//////////////////////////////////
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
							tag t_vert("vertex");
							t_vert.addAttribute("id", vertexList[a]);
						}
					}
				}

				//////////////////////////////////
				// the skinning info

				// looking for input msh and its skinning cluster
				MPlug sourceMesh = meshFn.findPlug("inMesh", &status);
				assert(status == MStatus::kSuccess);
				MObject skinObject;
                for(MItDependencyGraph iter(sourceMesh, MFn::kSkinClusterFilter, MItDependencyGraph::kUpstream, MItDependencyGraph::kBreadthFirst); !iter.isDone(); iter.next())
					skinObject = iter.currentItem();
				MFnSkinCluster skinFn(skinObject, &status);
				if(status != MStatus::kSuccess)
					mout << "  No skinning cluster found." << endl;
				else {
					mout << "  Skinning: " << skinFn.name() << endl;

					// influencing bones
					MDagPathArray boneDagPaths;
					const unsigned skinBoneCount = skinFn.influenceObjects(boneDagPaths, &status);
					assert(status == MStatus::kSuccess);

					{
						tag t_skin("skinning");
						mout << "    " << skinBoneCount << " bones" << endl;

						// iterate through the geometry and get the skinning weights
						MDoubleArray weights;
						unsigned boneCount;
						unsigned maxNonzeroCount = 0;
						unsigned singleWeightCount = 0;
						unsigned almostSingleWeightCount = 0;
						unsigned vertexCount = 0;
						for(MItGeometry geomIter(mesh); !geomIter.isDone(); geomIter.next()) {
							tag t_vertex("vertex");

							// get the component (vertex)
							MObject component = geomIter.component(&status);
							assert(status == MStatus::kSuccess);

							// get the weights
							status = skinFn.getWeights(mesh, component, weights, boneCount);
							assert(status == MStatus::kSuccess);

							// count the non-zero ones
							unsigned nonzeroCount = 0, almostNonzeroCount = 0;
							for(unsigned a=0;a<weights.length();a++) {
								if(weights[a] > 0.0) {
									tag t_weight("weight");
									nonzeroCount++;
									t_weight.addAttribute("bone", a);
									t_weight.addAttribute("weight", weights[a]);
								}
								if(weights[a] > 0.01)
									almostNonzeroCount++;
							}
							if(maxNonzeroCount < nonzeroCount)
								maxNonzeroCount = nonzeroCount;
							if(nonzeroCount == 1)
								singleWeightCount++;
							if(almostNonzeroCount == 1)
								almostSingleWeightCount++;
							vertexCount++;
						}
						mout << "    vertex count: " << vertexCount << endl;
						mout << "    single weighted: " << singleWeightCount << endl;
						mout << "    almost single weighted: " << almostSingleWeightCount << endl;
						mout << "    max weight count: " << maxNonzeroCount << endl;
					}

					//////////////////////////////////
					// skeleton gets exported only if a skinning cluster was found
					{
						mout << "  Skeleton: " << endl;
						tag t_skel("skeleton");

						// process all bones
						mout << "    " << boneDagPaths.length() << " bones" << endl;
						for(unsigned b=0;b<boneDagPaths.length();b++) {
							// get the bone node + name
							MFnDagNode nodeFn(boneDagPaths[b], &status);
							assert(status == MStatus::kSuccess);
							mout << "      bone " << b << ": " << nodeFn.name() << endl;
							tag t_bone("bone");
							t_bone.addAttribute("name", nodeFn.name().asChar());

							// process the transformation object
							MFnTransform transFn(boneDagPaths[b], &status);
							assert(status == MStatus::kSuccess);

							// construct the parent index
							//   - if no parent -> parent index = -1
							//   - if no parent found in the current set of bones -> set parent index to -1
							//   - if parent exists -> set the parentIndex to its index value
							assert(transFn.parentCount() <= 1);
							int parentIndex = -1;
							if(transFn.parentCount() > 0) {
								MObject par = transFn.parent(0, &status);
								assert(status == MStatus::kSuccess);
								MFnDagNode parentFn(par, &status);
								assert(status == MStatus::kSuccess);

								for(int a=0;a<(int)boneDagPaths.length();a++) {
									MFnDagNode testFn(boneDagPaths[a], &status);
									assert(status == MStatus::kSuccess);

									if(testFn.name() == parentFn.name()) {
										assert(parentIndex < 0);
										parentIndex = a;
									}
								}
							}
							mout << "        parent " << parentIndex << endl;
							t_bone.addAttribute("parent", parentIndex);

							// rotation
							{
								MQuaternion rot;
								status = transFn.getRotation(rot);
								assert(status == MStatus::kSuccess);
								//mout << "        rot " << rot.x << " " << rot.y << " " << rot.z << " " << rot.w << endl;
								tag t_rot("rotation");
								t_rot.addAttribute("x", rot.x);
								t_rot.addAttribute("y", rot.y);
								t_rot.addAttribute("z", rot.z);
								t_rot.addAttribute("w", rot.w);
							}

							// translation
							{
								MVector tr = transFn.getTranslation(MSpace::kObject, &status);
								assert(status == MStatus::kSuccess);
								//mout << "        tr " << tr.x << " " << tr.y << " " << tr.z << endl;
								tag t_tr("translation");
								t_tr.addAttribute("x", tr.x);
								t_tr.addAttribute("y", tr.y);
								t_tr.addAttribute("z", tr.z);
							}
						}
					}

					// put the mesh back to normal (assuming no animation was exported, which would do that somewhere above)
					//   according to http://nccastaff.bournemouth.ac.uk/jmacey/RobTheBloke/www/research/maya/mfnskinning.htm
					//   DOES IT WORK?!
					status = MGlobal::executeCommand("doEnableNodeItems true all");
					assert(status == MStatus::kSuccess);

					//////////////////////////////////
					// animation export
					{
						// put the model back to the animated state
						bindingPose(false);

						// get the number of frames
						mout << "  Animation - from " << MAnimControl::animationStartTime() << " to " << MAnimControl::animationEndTime() << endl;
						tag t_anim("animation");
						
						/*// save the frames
						for(MTime t = MAnimControl::animationStartTime(); t <= MAnimControl::animationEndTime(); t++) {
							MAnimControl::setCurrentTime(t);

							tag t_frame("frame");

							// go through all the bones
							for(unsigned b=0;b<boneDagPaths.length();b++) {
								tag t_bone("bone");

								// process the transformation object
								MFnTransform transFn(boneDagPaths[b], &status);
								assert(status == MStatus::kSuccess);

								// rotation
								{
									MQuaternion rot;
									status = transFn.getRotation(rot);
									assert(status == MStatus::kSuccess);
									//mout << "        rot " << rot.x << " " << rot.y << " " << rot.z << " " << rot.w << endl;
									tag t_rot("rotation");
									t_rot.addAttribute("x", rot.x);
									t_rot.addAttribute("y", rot.y);
									t_rot.addAttribute("z", rot.z);
									t_rot.addAttribute("w", rot.w);
								}

								// translation
								{
									MVector tr = transFn.getTranslation(MSpace::kObject, &status);
									assert(status == MStatus::kSuccess);
									//mout << "        tr " << tr.x << " " << tr.y << " " << tr.z << endl;
									tag t_tr("translation");
									t_tr.addAttribute("x", tr.x);
									t_tr.addAttribute("y", tr.y);
									t_tr.addAttribute("z", tr.z);
								}
							}
						}
						*/

						// go through all the bones
						for(unsigned b=0;b<boneDagPaths.length();b++) {
							tag t_bone("bone");

							for(MTime t = MAnimControl::animationStartTime(); t <= MAnimControl::animationEndTime(); t++) {
								MAnimControl::setCurrentTime(t);

								tag t_frame("frame");

								// process the transformation object
								MFnTransform transFn(boneDagPaths[b], &status);
								assert(status == MStatus::kSuccess);

								// rotation
								{
									MQuaternion rot;
									status = transFn.getRotation(rot);
									assert(status == MStatus::kSuccess);
									//mout << "        rot " << rot.x << " " << rot.y << " " << rot.z << " " << rot.w << endl;
									tag t_rot("rotation");
									t_rot.addAttribute("x", rot.x);
									t_rot.addAttribute("y", rot.y);
									t_rot.addAttribute("z", rot.z);
									t_rot.addAttribute("w", rot.w);
								}

								// translation
								{
									MVector tr = transFn.getTranslation(MSpace::kObject, &status);
									assert(status == MStatus::kSuccess);
									//mout << "        tr " << tr.x << " " << tr.y << " " << tr.z << endl;
									tag t_tr("translation");
									t_tr.addAttribute("x", tr.x);
									t_tr.addAttribute("y", tr.y);
									t_tr.addAttribute("z", tr.z);
								}
							}
						}
					}
				}
			}
		}
    }

	bindingPose(false);
	
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