/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#define REAL float
#define REAL2 float2
#define REAL4 float4
#define REAL8 float8
#define REAL16 float16
#define INT uint

#ifdef DIM_3
__constant sampler_t mySampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;


/* Intensity kernel */
__kernel void IntensityMatchingMetricSparseMask(
                                        __constant REAL4 * rigidTransform,
                                        __global REAL* movingInt, __global REAL4* movingLoc,
                                        read_only image3d_t fixedImage,
                                        __constant REAL4 * fixedPhysToIndTransform,
                                        __global REAL * metricOutput, 
                                        __local REAL * metricAccums
                                        )
{

  unsigned int gidx = get_global_id(0);
  unsigned int lid = get_local_id(0);
  unsigned int groupID = get_group_id(0);                                      

  /* Evaluate Fixed Image Gradient */  
  REAL4 mLoc = movingLoc[gidx];
  REAL  mInt = movingInt[gidx];

  REAL4 rcX = rigidTransform[0];
  REAL4 rcY = rigidTransform[1];
  REAL4 rcZ = rigidTransform[2];

  REAL4 cId = (REAL4)(0.0f, 0.0f, 0.0f, 0.0f);
  cId.x = dot(rcX, mLoc);
  cId.y = dot(rcY, mLoc);
  cId.z = dot(rcZ, mLoc);

  REAL4 phys2IndRotX = fixedPhysToIndTransform[0];
  REAL4 phys2IndRotY = fixedPhysToIndTransform[1];
  REAL4 phys2IndRotZ = fixedPhysToIndTransform[2];

  REAL4 phys2IndTranslation = (REAL4)(0.0f, 0.0f, 0.0f, 0.0f);
  phys2IndTranslation.x = phys2IndRotX.w;
  phys2IndTranslation.y = phys2IndRotY.w;
  phys2IndTranslation.z = phys2IndRotZ.w;
  REAL4 po = cId - phys2IndTranslation;
  po.w = 0.0f;

  REAL4 fixedIndex = (REAL4)(0.0f, 0.0f, 0.0f, 0.0f);
  fixedIndex.x = dot(phys2IndRotX, po);
  fixedIndex.y = dot(phys2IndRotY, po);
  fixedIndex.z = dot(phys2IndRotZ, po);

  REAL4 fixedInt = read_imagef(fixedImage, mySampler, fixedIndex);

  REAL metricValue = fixedInt.x * mInt;

  metricAccums[lid] = metricValue;  
  
  barrier(CLK_LOCAL_MEM_FENCE);

  for(unsigned int s = LOCALSIZE / 2; s > 0; s >>= 1)
  {
    if(lid < s)
    {
      metricAccums[lid] += metricAccums[lid+s];
    }
    barrier(CLK_LOCAL_MEM_FENCE);
  }
  
  if(lid == 0) metricOutput[groupID] = metricAccums[0];
}

#endif
