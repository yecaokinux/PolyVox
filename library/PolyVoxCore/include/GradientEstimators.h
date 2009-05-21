#pragma region License
/******************************************************************************
This file is part of the PolyVox library
Copyright (C) 2006  David Williams

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
******************************************************************************/
#pragma endregion

#ifndef __PolyVox_GradientEstimators_H__
#define __PolyVox_GradientEstimators_H__

#include "VolumeSampler.h"

#include <vector>

namespace PolyVox
{
	enum NormalGenerationMethod
	{
		SIMPLE, ///<Fastest
		CENTRAL_DIFFERENCE,
		SOBEL,
		CENTRAL_DIFFERENCE_SMOOTHED,
		SOBEL_SMOOTHED ///<Smoothest
	};

	template <typename VoxelType>
	Vector3DFloat computeCentralDifferenceGradient(const VolumeSampler<VoxelType>& volIter);

	template <typename VoxelType>
	Vector3DFloat computeSmoothCentralDifferenceGradient(VolumeSampler<VoxelType>& volIter);

	template <typename VoxelType>
	Vector3DFloat computeDecimatedCentralDifferenceGradient(VolumeSampler<VoxelType>& volIter);

	template <typename VoxelType>
	Vector3DFloat computeSobelGradient(const VolumeSampler<VoxelType>& volIter);
	template <typename VoxelType>
	Vector3DFloat computeSmoothSobelGradient(VolumeSampler<VoxelType>& volIter);

	POLYVOXCORE_API void computeNormalsForVertices(Volume<uint8_t>* volumeData, IndexedSurfacePatch& isp, NormalGenerationMethod normalGenerationMethod);
	POLYVOXCORE_API Vector3DFloat computeNormal(Volume<uint8_t>* volumeData, const Vector3DFloat& v3dPos, NormalGenerationMethod normalGenerationMethod);
}

#include "GradientEstimators.inl"

#endif //__PolyVox_GradientEstimators_H__
