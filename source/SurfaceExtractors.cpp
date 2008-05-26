#include "SurfaceExtractors.h"

#include "BlockVolume.h"
#include "GradientEstimators.h"
#include "IndexedSurfacePatch.h"
#include "MarchingCubesTables.h"
#include "Region.h"
#include "RegionGeometry.h"
#include "VolumeChangeTracker.h"
#include "BlockVolumeIterator.h"

using namespace boost;

namespace PolyVox
{
	std::list<RegionGeometry> getChangedRegionGeometry(VolumeChangeTracker& volume)
	{
		std::list<Region> listChangedRegions;
		volume.getChangedRegions(listChangedRegions);

		std::list<RegionGeometry> listChangedRegionGeometry;
		for(std::list<Region>::const_iterator iterChangedRegions = listChangedRegions.begin(); iterChangedRegions != listChangedRegions.end(); ++iterChangedRegions)
		{
			//Generate the surface
			RegionGeometry regionGeometry;
			regionGeometry.m_patchSingleMaterial = new IndexedSurfacePatch(false);
			regionGeometry.m_patchMultiMaterial = new IndexedSurfacePatch(true);
			regionGeometry.m_v3dRegionPosition = iterChangedRegions->getLowerCorner();

			generateRoughMeshDataForRegion(volume.getVolumeData(), *iterChangedRegions, regionGeometry.m_patchSingleMaterial, regionGeometry.m_patchMultiMaterial);

			regionGeometry.m_bContainsSingleMaterialPatch = regionGeometry.m_patchSingleMaterial->getVertices().size() > 0;
			regionGeometry.m_bContainsMultiMaterialPatch = regionGeometry.m_patchMultiMaterial->getVertices().size() > 0;
			regionGeometry.m_bIsEmpty = ((regionGeometry.m_patchSingleMaterial->getVertices().size() == 0) && (regionGeometry.m_patchMultiMaterial->getIndices().size() == 0));

			listChangedRegionGeometry.push_back(regionGeometry);
		}

		return listChangedRegionGeometry;
	}

	void generateRoughMeshDataForRegion(BlockVolume<uint8_t>* volumeData, Region region, IndexedSurfacePatch* singleMaterialPatch, IndexedSurfacePatch* multiMaterialPatch)
	{	
		//When generating the mesh for a region we actually look one voxel outside it in the
		// back, bottom, right direction. Protect against access violations by cropping region here
		Region regVolume = volumeData->getEnclosingRegion();
		regVolume.setUpperCorner(regVolume.getUpperCorner() - Vector3DInt32(1,1,1));
		region.cropTo(regVolume);

		//Offset from lower block corner
		const Vector3DFloat offset = static_cast<Vector3DFloat>(region.getLowerCorner());

		Vector3DFloat vertlist[12];
		uint8_t vertMaterials[12];
		BlockVolumeIterator<boost::uint8_t> volIter(*volumeData);
		volIter.setValidRegion(region);

		//////////////////////////////////////////////////////////////////////////
		//Get mesh data
		//////////////////////////////////////////////////////////////////////////

		//Iterate over each cell in the region
		for(volIter.setPosition(region.getLowerCorner().getX(),region.getLowerCorner().getY(), region.getLowerCorner().getZ());volIter.isValidForRegion();volIter.moveForwardInRegion())
		{		
			//Current position
			const uint16_t x = volIter.getPosX();
			const uint16_t y = volIter.getPosY();
			const uint16_t z = volIter.getPosZ();

			//Voxels values
			const uint8_t v000 = volIter.getVoxel();
			const uint8_t v100 = volIter.peekVoxel1px0py0pz();
			const uint8_t v010 = volIter.peekVoxel0px1py0pz();
			const uint8_t v110 = volIter.peekVoxel1px1py0pz();
			const uint8_t v001 = volIter.peekVoxel0px0py1pz();
			const uint8_t v101 = volIter.peekVoxel1px0py1pz();
			const uint8_t v011 = volIter.peekVoxel0px1py1pz();
			const uint8_t v111 = volIter.peekVoxel1px1py1pz();

			//Determine the index into the edge table which tells us which vertices are inside of the surface
			uint8_t iCubeIndex = 0;

			if (v000 == 0) iCubeIndex |= 1;
			if (v100 == 0) iCubeIndex |= 2;
			if (v110 == 0) iCubeIndex |= 4;
			if (v010 == 0) iCubeIndex |= 8;
			if (v001 == 0) iCubeIndex |= 16;
			if (v101 == 0) iCubeIndex |= 32;
			if (v111 == 0) iCubeIndex |= 64;
			if (v011 == 0) iCubeIndex |= 128;

			/* Cube is entirely in/out of the surface */
			if (edgeTable[iCubeIndex] == 0)
			{
				continue;
			}

			/* Find the vertices where the surface intersects the cube */
			if (edgeTable[iCubeIndex] & 1)
			{
				vertlist[0].setX(x + 0.5f);
				vertlist[0].setY(y);
				vertlist[0].setZ(z);
				vertMaterials[0] = v000 | v100; //Because one of these is 0, the or operation takes the max.
			}
			if (edgeTable[iCubeIndex] & 2)
			{
				vertlist[1].setX(x + 1.0f);
				vertlist[1].setY(y + 0.5f);
				vertlist[1].setZ(z);
				vertMaterials[1] = v100 | v110;
			}
			if (edgeTable[iCubeIndex] & 4)
			{
				vertlist[2].setX(x + 0.5f);
				vertlist[2].setY(y + 1.0f);
				vertlist[2].setZ(z);
				vertMaterials[2] = v010 | v110;
			}
			if (edgeTable[iCubeIndex] & 8)
			{
				vertlist[3].setX(x);
				vertlist[3].setY(y + 0.5f);
				vertlist[3].setZ(z);
				vertMaterials[3] = v000 | v010;
			}
			if (edgeTable[iCubeIndex] & 16)
			{
				vertlist[4].setX(x + 0.5f);
				vertlist[4].setY(y);
				vertlist[4].setZ(z + 1.0f);
				vertMaterials[4] = v001 | v101;
			}
			if (edgeTable[iCubeIndex] & 32)
			{
				vertlist[5].setX(x + 1.0f);
				vertlist[5].setY(y + 0.5f);
				vertlist[5].setZ(z + 1.0f);
				vertMaterials[5] = v101 | v111;
			}
			if (edgeTable[iCubeIndex] & 64)
			{
				vertlist[6].setX(x + 0.5f);
				vertlist[6].setY(y + 1.0f);
				vertlist[6].setZ(z + 1.0f);
				vertMaterials[6] = v011 | v111;
			}
			if (edgeTable[iCubeIndex] & 128)
			{
				vertlist[7].setX(x);
				vertlist[7].setY(y + 0.5f);
				vertlist[7].setZ(z + 1.0f);
				vertMaterials[7] = v001 | v011;
			}
			if (edgeTable[iCubeIndex] & 256)
			{
				vertlist[8].setX(x);
				vertlist[8].setY(y);
				vertlist[8].setZ(z + 0.5f);
				vertMaterials[8] = v000 | v001;
			}
			if (edgeTable[iCubeIndex] & 512)
			{
				vertlist[9].setX(x + 1.0f);
				vertlist[9].setY(y);
				vertlist[9].setZ(z + 0.5f);
				vertMaterials[9] = v100 | v101;
			}
			if (edgeTable[iCubeIndex] & 1024)
			{
				vertlist[10].setX(x + 1.0f);
				vertlist[10].setY(y + 1.0f);
				vertlist[10].setZ(z + 0.5f);
				vertMaterials[10] = v110 | v111;
			}
			if (edgeTable[iCubeIndex] & 2048)
			{
				vertlist[11].setX(x);
				vertlist[11].setY(y + 1.0f);
				vertlist[11].setZ(z + 0.5f);
				vertMaterials[11] = v010 | v011;
			}

			for (int i=0;triTable[iCubeIndex][i]!=-1;i+=3)
			{
				//The three vertices forming a triangle
				const Vector3DFloat vertex0 = vertlist[triTable[iCubeIndex][i  ]] - offset;
				const Vector3DFloat vertex1 = vertlist[triTable[iCubeIndex][i+1]] - offset;
				const Vector3DFloat vertex2 = vertlist[triTable[iCubeIndex][i+2]] - offset;

				//Cast to floats and divide by two.
				//const Vector3DFloat vertex0AsFloat = (static_cast<Vector3DFloat>(vertex0) / 2.0f) - offset;
				//const Vector3DFloat vertex1AsFloat = (static_cast<Vector3DFloat>(vertex1) / 2.0f) - offset;
				//const Vector3DFloat vertex2AsFloat = (static_cast<Vector3DFloat>(vertex2) / 2.0f) - offset;

				const uint8_t material0 = vertMaterials[triTable[iCubeIndex][i  ]];
				const uint8_t material1 = vertMaterials[triTable[iCubeIndex][i+1]];
				const uint8_t material2 = vertMaterials[triTable[iCubeIndex][i+2]];


				//If all the materials are the same, we just need one triangle for that material with all the alphas set high.
				if((material0 == material1) && (material1 == material2))
				{
					SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1f,1.0f);
					SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1f,1.0f);
					SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1f,1.0f);
					singleMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
				}
				else if(material0 == material1)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1f,1.0f);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1f,1.0f);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1f,0.0f);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material2 + 0.1f,0.0f);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material2 + 0.1f,0.0f);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1f,1.0f);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else if(material0 == material2)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1f,1.0f);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1f,0.0f);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1f,1.0f);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1f,0.0f);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1f,1.0f);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1f,0.0f);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else if(material1 == material2)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1f,0.0f);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1f,1.0f);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1f,1.0f);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1f,1.0f);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1f,0.0f);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1f,0.0f);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1f,1.0f);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1f,0.0f);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1f,0.0f);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1f,0.0f);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1f,1.0f);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1f,0.0f);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material2 + 0.1f,0.0f);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material2 + 0.1f,0.0f);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1f,1.0f);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				//If there not all the same, we need one triangle for each unique material.
				//We'll also need some vertices with low alphas for blending.
				/*else 
				{
				SurfaceVertex surfaceVertex0Alpha0(vertex0,0.0);
				SurfaceVertex surfaceVertex1Alpha0(vertex1,0.0);
				SurfaceVertex surfaceVertex2Alpha0(vertex2,0.0);

				if(material0 == material1)
				{
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha0);
				surfacePatchMapResult[material2]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha0, surfaceVertex2Alpha1);
				}
				else if(material1 == material2)
				{
				surfacePatchMapResult[material1]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha0, surfaceVertex2Alpha0);
				}
				else if(material2 == material0)
				{
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha0, surfaceVertex2Alpha1);
				surfacePatchMapResult[material1]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha1, surfaceVertex2Alpha0);
				}
				else
				{
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha0, surfaceVertex2Alpha0);
				surfacePatchMapResult[material1]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha1, surfaceVertex2Alpha0);
				surfacePatchMapResult[material2]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha0, surfaceVertex2Alpha1);
				}
				}*/
			}//For each triangle
		}//For each cell

		//FIXME - can it happen that we have no vertices or triangles? Should exit early?


		//for(std::map<uint8_t, IndexedSurfacePatch*>::iterator iterPatch = surfacePatchMapResult.begin(); iterPatch != surfacePatchMapResult.end(); ++iterPatch)
		{

			std::vector<SurfaceVertex>::iterator iterSurfaceVertex = singleMaterialPatch->getVertices().begin();
			while(iterSurfaceVertex != singleMaterialPatch->getVertices().end())
			{
				Vector3DFloat tempNormal = computeNormal(volumeData, static_cast<Vector3DFloat>(iterSurfaceVertex->getPosition() + offset), CENTRAL_DIFFERENCE);
				const_cast<SurfaceVertex&>(*iterSurfaceVertex).setNormal(tempNormal);
				++iterSurfaceVertex;
			}

			iterSurfaceVertex = multiMaterialPatch->getVertices().begin();
			while(iterSurfaceVertex != multiMaterialPatch->getVertices().end())
			{
				Vector3DFloat tempNormal = computeNormal(volumeData, static_cast<Vector3DFloat>(iterSurfaceVertex->getPosition() + offset), CENTRAL_DIFFERENCE);
				const_cast<SurfaceVertex&>(*iterSurfaceVertex).setNormal(tempNormal);
				++iterSurfaceVertex;
			}

			uint16_t noOfRemovedVertices = 0;
			//do
			{
				//noOfRemovedVertices = iterPatch->second.decimate();
			}
			//while(noOfRemovedVertices > 10); //We don't worry about the last few vertices - it's not worth the overhead of calling the function.
		}

		//return singleMaterialPatch;
	}

	Vector3DFloat computeNormal(BlockVolume<uint8_t>* volumeData, const Vector3DFloat& position, NormalGenerationMethod normalGenerationMethod)
	{
		const float posX = position.getX();
		const float posY = position.getY();
		const float posZ = position.getZ();

		const uint16_t floorX = static_cast<uint16_t>(posX);
		const uint16_t floorY = static_cast<uint16_t>(posY);
		const uint16_t floorZ = static_cast<uint16_t>(posZ);

		//Check all corners are within the volume, allowing a boundary for gradient estimation
		bool lowerCornerInside = volumeData->containsPoint(Vector3DInt32(floorX, floorY, floorZ),1);
		bool upperCornerInside = volumeData->containsPoint(Vector3DInt32(floorX+1, floorY+1, floorZ+1),1);
		if((!lowerCornerInside) || (!upperCornerInside))
		{
			normalGenerationMethod = SIMPLE;
		}

		Vector3DFloat result;

		BlockVolumeIterator<boost::uint8_t> volIter(*volumeData); //FIXME - save this somewhere - could be expensive to create?


		if(normalGenerationMethod == SOBEL)
		{
			volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			const Vector3DFloat gradFloor = computeSobelGradient(volIter);
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX+1.0),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			}
			if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY+1.0),static_cast<uint16_t>(posZ));
			}
			if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ+1.0));					
			}
			const Vector3DFloat gradCeil = computeSobelGradient(volIter);
			result = ((gradFloor + gradCeil) * -1.0f);
			if(result.lengthSquared() < 0.0001)
			{
				//Operation failed - fall back on simple gradient estimation
				normalGenerationMethod = SIMPLE;
			}
		}
		if(normalGenerationMethod == CENTRAL_DIFFERENCE)
		{
			volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			const Vector3DFloat gradFloor = computeCentralDifferenceGradient(volIter);
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX+1.0),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			}
			if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY+1.0),static_cast<uint16_t>(posZ));
			}
			if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ+1.0));					
			}
			const Vector3DFloat gradCeil = computeCentralDifferenceGradient(volIter);
			result = ((gradFloor + gradCeil) * -1.0f);
			if(result.lengthSquared() < 0.0001)
			{
				//Operation failed - fall back on simple gradient estimation
				normalGenerationMethod = SIMPLE;
			}
		}
		if(normalGenerationMethod == SIMPLE)
		{
			volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			const uint8_t uFloor = volIter.getVoxel() > 0 ? 1 : 0;
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{					
				uint8_t uCeil = volIter.peekVoxel1px0py0pz() > 0 ? 1 : 0;
				result = Vector3DFloat(static_cast<float>(uFloor - uCeil),0.0,0.0);
			}
			else if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{
				uint8_t uCeil = volIter.peekVoxel0px1py0pz() > 0 ? 1 : 0;
				result = Vector3DFloat(0.0,static_cast<float>(uFloor - uCeil),0.0);
			}
			else if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{
				uint8_t uCeil = volIter.peekVoxel0px0py1pz() > 0 ? 1 : 0;
				result = Vector3DFloat(0.0, 0.0,static_cast<float>(uFloor - uCeil));					
			}
		}
		return result;
	}

	void generateSmoothMeshDataForRegion(BlockVolume<uint8_t>* volumeData, Region region, IndexedSurfacePatch* singleMaterialPatch, IndexedSurfacePatch* multiMaterialPatch)
	{	
		//When generating the mesh for a region we actually look one voxel outside it in the
		// back, bottom, right direction. Protect against access violations by cropping region here
		Region regVolume = volumeData->getEnclosingRegion();
		regVolume.setUpperCorner(regVolume.getUpperCorner() - Vector3DInt32(1,1,1));
		region.cropTo(regVolume);

		//Offset from lower block corner
		const Vector3DFloat offset = static_cast<Vector3DFloat>(region.getLowerCorner());

		Vector3DFloat vertlist[12];
		uint8_t vertMaterials[12];
		BlockVolumeIterator<boost::uint8_t> volIter(*volumeData);
		volIter.setValidRegion(region);

		const float threshold = 0.5f;

		//////////////////////////////////////////////////////////////////////////
		//Get mesh data
		//////////////////////////////////////////////////////////////////////////

		//Iterate over each cell in the region
		for(volIter.setPosition(region.getLowerCorner().getX(),region.getLowerCorner().getY(), region.getLowerCorner().getZ());volIter.isValidForRegion();volIter.moveForwardInRegion())
		{		
			//Current position
			const uint16_t x = volIter.getPosX();
			const uint16_t y = volIter.getPosY();
			const uint16_t z = volIter.getPosZ();

			//Voxels values
			BlockVolumeIterator<boost::uint8_t> tempVolIter(*volumeData);
			tempVolIter.setPosition(x,y,z);
			const float v000 = tempVolIter.getAveragedVoxel(1);
			tempVolIter.setPosition(x+1,y,z);
			const float v100 = tempVolIter.getAveragedVoxel(1);
			tempVolIter.setPosition(x,y+1,z);
			const float v010 = tempVolIter.getAveragedVoxel(1);
			tempVolIter.setPosition(x+1,y+1,z);
			const float v110 = tempVolIter.getAveragedVoxel(1);
			tempVolIter.setPosition(x,y,z+1);
			const float v001 = tempVolIter.getAveragedVoxel(1);
			tempVolIter.setPosition(x+1,y,z+1);
			const float v101 = tempVolIter.getAveragedVoxel(1);
			tempVolIter.setPosition(x,y+1,z+1);
			const float v011 = tempVolIter.getAveragedVoxel(1);
			tempVolIter.setPosition(x+1,y+1,z+1);
			const float v111 = tempVolIter.getAveragedVoxel(1);

			//Determine the index into the edge table which tells us which vertices are inside of the surface
			uint8_t iCubeIndex = 0;

			if (v000 < threshold) iCubeIndex |= 1;
			if (v100 < threshold) iCubeIndex |= 2;
			if (v110 < threshold) iCubeIndex |= 4;
			if (v010 < threshold) iCubeIndex |= 8;
			if (v001 < threshold) iCubeIndex |= 16;
			if (v101 < threshold) iCubeIndex |= 32;
			if (v111 < threshold) iCubeIndex |= 64;
			if (v011 < threshold) iCubeIndex |= 128;

			/* Cube is entirely in/out of the surface */
			if (edgeTable[iCubeIndex] == 0)
			{
				continue;
			}

			/* Find the vertices where the surface intersects the cube */
			if (edgeTable[iCubeIndex] & 1)
			{
				float a = v000;
				float b = v100;
				float val = (threshold-a)/(b-a);
				vertlist[0].setX(x + val);
				vertlist[0].setY(y);
				vertlist[0].setZ(z);
				vertMaterials[0] = 1;//v000 | v100; //Because one of these is 0, the or operation takes the max.
			}
			if (edgeTable[iCubeIndex] & 2)
			{
				float a = v100;
				float b = v110;
				float val = (threshold-a)/(b-a);
				vertlist[1].setX(x + 1.0f);
				vertlist[1].setY(y + val);
				vertlist[1].setZ(z);
				vertMaterials[1] = 1;//v100 | v110;
			}
			if (edgeTable[iCubeIndex] & 4)
			{
				float a = v010;
				float b = v110;
				float val = (threshold-a)/(b-a);
				vertlist[2].setX(x + val);
				vertlist[2].setY(y + 1.0f);
				vertlist[2].setZ(z);
				vertMaterials[2] = 1;//v010 | v110;
			}
			if (edgeTable[iCubeIndex] & 8)
			{
				float a = v000;
				float b = v010;
				float val = (threshold-a)/(b-a);
				vertlist[3].setX(x);
				vertlist[3].setY(y + val);
				vertlist[3].setZ(z);
				vertMaterials[3] = 1;//v000 | v010;
			}
			if (edgeTable[iCubeIndex] & 16)
			{
				float a = v001;
				float b = v101;
				float val = (threshold-a)/(b-a);
				vertlist[4].setX(x + val);
				vertlist[4].setY(y);
				vertlist[4].setZ(z + 1.0f);
				vertMaterials[4] = 1;//v001 | v101;
			}
			if (edgeTable[iCubeIndex] & 32)
			{
				float a = v101;
				float b = v111;
				float val = (threshold-a)/(b-a);
				vertlist[5].setX(x + 1.0f);
				vertlist[5].setY(y + val);
				vertlist[5].setZ(z + 1.0f);
				vertMaterials[5] = 1;//v101 | v111;
			}
			if (edgeTable[iCubeIndex] & 64)
			{
				float a = v011;
				float b = v111;
				float val = (threshold-a)/(b-a);
				vertlist[6].setX(x + val);
				vertlist[6].setY(y + 1.0f);
				vertlist[6].setZ(z + 1.0f);
				vertMaterials[6] = 1;//v011 | v111;
			}
			if (edgeTable[iCubeIndex] & 128)
			{
				float a = v001;
				float b = v011;
				float val = (threshold-a)/(b-a);
				vertlist[7].setX(x);
				vertlist[7].setY(y + val);
				vertlist[7].setZ(z + 1.0f);
				vertMaterials[7] = 1;//v001 | v011;
			}
			if (edgeTable[iCubeIndex] & 256)
			{
				float a = v000;
				float b = v001;
				float val = (threshold-a)/(b-a);
				vertlist[8].setX(x);
				vertlist[8].setY(y);
				vertlist[8].setZ(z + val);
				vertMaterials[8] = 1;//v000 | v001;
			}
			if (edgeTable[iCubeIndex] & 512)
			{
				float a = v100;
				float b = v101;
				float val = (threshold-a)/(b-a);
				vertlist[9].setX(x + 1.0f);
				vertlist[9].setY(y);
				vertlist[9].setZ(z + val);
				vertMaterials[9] = 1;//v100 | v101;
			}
			if (edgeTable[iCubeIndex] & 1024)
			{
				float a = v110;
				float b = v111;
				float val = (threshold-a)/(b-a);
				vertlist[10].setX(x + 1.0f);
				vertlist[10].setY(y + 1.0f);
				vertlist[10].setZ(z + val);
				vertMaterials[10] = 1;//v110 | v111;
			}
			if (edgeTable[iCubeIndex] & 2048)
			{
				float a = v010;
				float b = v011;
				float val = (threshold-a)/(b-a);
				vertlist[11].setX(x);
				vertlist[11].setY(y + 1.0f);
				vertlist[11].setZ(z + val);
				vertMaterials[11] = 1;//v010 | v011;
			}

			for (int i=0;triTable[iCubeIndex][i]!=-1;i+=3)
			{
				//The three vertices forming a triangle
				const Vector3DFloat vertex0 = vertlist[triTable[iCubeIndex][i  ]] - offset;
				const Vector3DFloat vertex1 = vertlist[triTable[iCubeIndex][i+1]] - offset;
				const Vector3DFloat vertex2 = vertlist[triTable[iCubeIndex][i+2]] - offset;

				const uint8_t material0 = vertMaterials[triTable[iCubeIndex][i  ]];
				const uint8_t material1 = vertMaterials[triTable[iCubeIndex][i+1]];
				const uint8_t material2 = vertMaterials[triTable[iCubeIndex][i+2]];

				//If all the materials are the same, we just need one triangle for that material with all the alphas set high.
				/*if((material0 == material1) && (material1 == material2))
				{*/
					SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1f,1.0f);
					SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1f,1.0f);
					SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1f,1.0f);
					singleMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
				/*}
				else if(material0 == material1)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else if(material0 == material2)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else if(material1 == material2)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}*/				
			}//For each triangle
		}//For each cell

		//FIXME - can it happen that we have no vertices or triangles? Should exit early?


		//for(std::map<uint8_t, IndexedSurfacePatch*>::iterator iterPatch = surfacePatchMapResult.begin(); iterPatch != surfacePatchMapResult.end(); ++iterPatch)
		{

			std::vector<SurfaceVertex>::iterator iterSurfaceVertex = singleMaterialPatch->getVertices().begin();
			while(iterSurfaceVertex != singleMaterialPatch->getVertices().end())
			{
				Vector3DFloat tempNormal = computeSmoothNormal(volumeData, static_cast<Vector3DFloat>(iterSurfaceVertex->getPosition() + offset), CENTRAL_DIFFERENCE);
				const_cast<SurfaceVertex&>(*iterSurfaceVertex).setNormal(tempNormal);
				++iterSurfaceVertex;
			}

			iterSurfaceVertex = multiMaterialPatch->getVertices().begin();
			while(iterSurfaceVertex != multiMaterialPatch->getVertices().end())
			{
				Vector3DFloat tempNormal = computeSmoothNormal(volumeData, static_cast<Vector3DFloat>(iterSurfaceVertex->getPosition() + offset), CENTRAL_DIFFERENCE);
				const_cast<SurfaceVertex&>(*iterSurfaceVertex).setNormal(tempNormal);
				++iterSurfaceVertex;
			}
		}
	}

	Vector3DFloat computeSmoothNormal(BlockVolume<uint8_t>* volumeData, const Vector3DFloat& position, NormalGenerationMethod normalGenerationMethod)
	{
		

		const float posX = position.getX();
		const float posY = position.getY();
		const float posZ = position.getZ();

		const uint16_t floorX = static_cast<uint16_t>(posX);
		const uint16_t floorY = static_cast<uint16_t>(posY);
		const uint16_t floorZ = static_cast<uint16_t>(posZ);

		//Check all corners are within the volume, allowing a boundary for gradient estimation
		bool lowerCornerInside = volumeData->containsPoint(Vector3DInt32(floorX, floorY, floorZ),1);
		bool upperCornerInside = volumeData->containsPoint(Vector3DInt32(floorX+1, floorY+1, floorZ+1),1);
		if((!lowerCornerInside) || (!upperCornerInside))
		{
			normalGenerationMethod = SIMPLE;
		}

		Vector3DFloat result;

		BlockVolumeIterator<boost::uint8_t> volIter(*volumeData); //FIXME - save this somewhere - could be expensive to create?


		if(normalGenerationMethod == SOBEL)
		{
			volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			const Vector3DFloat gradFloor = computeSobelGradient(volIter);
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX+1.0),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			}
			if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY+1.0),static_cast<uint16_t>(posZ));
			}
			if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ+1.0));					
			}
			const Vector3DFloat gradCeil = computeSobelGradient(volIter);
			result = ((gradFloor + gradCeil) * -1.0f);
			if(result.lengthSquared() < 0.0001)
			{
				//Operation failed - fall back on simple gradient estimation
				normalGenerationMethod = SIMPLE;
			}
		}
		if(normalGenerationMethod == CENTRAL_DIFFERENCE)
		{
			volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			const Vector3DFloat gradFloor = computeSmoothCentralDifferenceGradient(volIter);
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX+1.0),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			}
			if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY+1.0),static_cast<uint16_t>(posZ));
			}
			if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ+1.0));					
			}
			const Vector3DFloat gradCeil = computeSmoothCentralDifferenceGradient(volIter);
			result = ((gradFloor + gradCeil) * -1.0f);
			if(result.lengthSquared() < 0.0001)
			{
				//Operation failed - fall back on simple gradient estimation
				normalGenerationMethod = SIMPLE;
			}
		}
		if(normalGenerationMethod == SIMPLE)
		{
			volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			const uint8_t uFloor = volIter.getVoxel() > 0 ? 1 : 0;
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{					
				uint8_t uCeil = volIter.peekVoxel1px0py0pz() > 0 ? 1 : 0;
				result = Vector3DFloat(static_cast<float>(uFloor - uCeil),0.0,0.0);
			}
			else if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{
				uint8_t uCeil = volIter.peekVoxel0px1py0pz() > 0 ? 1 : 0;
				result = Vector3DFloat(0.0,static_cast<float>(uFloor - uCeil),0.0);
			}
			else if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{
				uint8_t uCeil = volIter.peekVoxel0px0py1pz() > 0 ? 1 : 0;
				result = Vector3DFloat(0.0, 0.0,static_cast<float>(uFloor - uCeil));					
			}
		}
		return result;
	}
}
