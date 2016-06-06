/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Dante De Nigris for writing this class

#define REAL float
#define REAL2 float2
#define REAL4 float4
#define REAL8 float8
#define REAL16 float16
#define INT uint
#define INT4 uint4
#define INT2 uint2

__kernel void OrientationMatchingMetricSparseMask(
                                        __constant REAL4 * rigidContext,
                                        __global REAL2* g_sliceGradients, 
                                        __global INT4* g_Indices,
                                        read_only image3d_t mgImage,
                                        __global REAL * metricOutput,
                                        __local REAL * metricAccums,
                                        __global REAL4 * g_IndexToWorldTransformations
                                        )
{

  __local REAL4 localIndexToWorldTransformations[SIZE_OF_TRANSFORMATION_SET];

  unsigned int gidx = get_global_id(0);
  unsigned int lid = get_local_id(0);
  unsigned int groupID = get_group_id(0);

  /* Load US Image Index 2D (row, column) and Slice Index */
  INT4 sliceIndexAndImageIndex = g_Indices[gidx];
  INT2 imageIndex = sliceIndexAndImageIndex.xy;
  INT  sliceIndex = sliceIndexAndImageIndex.z;

  event_t copydone = async_work_group_copy(localIndexToWorldTransformations, g_IndexToWorldTransformations, SIZE_OF_TRANSFORMATION_SET, 0);

  const sampler_t mySampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

  REAL4 rcX = rigidContext[0];
  REAL4 rcY = rigidContext[1];
  REAL4 rcZ = rigidContext[2];

  REAL4 imageIndex4;
  imageIndex4.x = (REAL)(imageIndex.x);
  imageIndex4.y = (REAL)(imageIndex.y);
  imageIndex4.z = (REAL)(0.0f);
  imageIndex4.w = (REAL)(1.0f);

  wait_group_events(1, &copydone);

  REAL4 TrpX = localIndexToWorldTransformations[3*sliceIndex + 0];
  REAL4 TrpY = localIndexToWorldTransformations[3*sliceIndex + 1];
  REAL4 TrpZ = localIndexToWorldTransformations[3*sliceIndex + 2];  

  REAL4 loc = (REAL4)(0.0f, 0.0f, 0.0f, 0.0f);
  loc.x = dot(TrpX, imageIndex4);
  loc.y = dot(TrpY, imageIndex4);
  loc.z = dot(TrpZ, imageIndex4);
  loc.w = (REAL)1.0;

  REAL4 cId = (REAL4)(0.5f, 0.5f, 0.5f, 0.0f);
  cId.x += dot(rcX, loc);
  cId.y += dot(rcY, loc);
  cId.z += dot(rcZ, loc);

  /* Interpolated Moving Gradient */
  REAL4 movingGrad = read_imagef(mgImage, mySampler, cId);

  REAL4 rctX = rigidContext[3];
  REAL4 rctY = rigidContext[4];
  REAL4 rctZ = rigidContext[5];

  /* Transformed Moving Gradient */
  REAL4 trMovingGrad = (REAL4)(0.0f, 0.0f, 0.0f, 0.0f);
  trMovingGrad.x = dot(rctX, movingGrad);
  trMovingGrad.y = dot(rctY, movingGrad);
  trMovingGrad.z = dot(rctZ, movingGrad);

  REAL4 trMovingGradN = normalize(trMovingGrad);

  REAL2 fixedGrad = g_sliceGradients[gidx];

  REAL4 base1 = (REAL4)(TrpX.x, TrpY.x, TrpZ.x, 0.0f);
  REAL4 base2 = (REAL4)(TrpX.y, TrpY.y, TrpZ.y, 0.0f);  

  REAL2 fixedGradN = normalize(fixedGrad);

  REAL2  projectedMovingGrad = (REAL2)(0.0f, 0.0f);
  projectedMovingGrad.x = dot(trMovingGradN, base1);
  projectedMovingGrad.y = dot(trMovingGradN, base2);

  REAL2 projectedMovingGradN = normalize(projectedMovingGrad);

  REAL innerProduct = dot(fixedGradN, projectedMovingGradN);
  REAL metricValue = pown(innerProduct, SEL);

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
