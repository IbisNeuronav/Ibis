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

#ifdef VERSION_A
__kernel void OrientationMatchingMetricSparseMask( 
                                        __constant REAL4 * rigidContext,
                                        __global REAL4* g_fg, __global REAL4* g_fl,
                                        read_only image3d_t mgImage,
                                        __global REAL * metricOutput, 
                                        __local REAL * metricAccums
                                        )
{

  unsigned int gidx = get_global_id(0);
  unsigned int lid = get_local_id(0);
  unsigned int groupID = get_group_id(0);                                      

  /* Evaluate Fixed Image Gradient */  
  REAL4 loc = g_fl[gidx];

  const sampler_t mySampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

  REAL4 rcX = rigidContext[0];
  REAL4 rcY = rigidContext[1];
  REAL4 rcZ = rigidContext[2];

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

  REAL4 fixedGrad = g_fg[gidx];      
  REAL4 fixedGradN = normalize(fixedGrad);

  REAL innerProduct = dot(fixedGradN, trMovingGradN);
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
#endif

#ifdef VERSION_B
__kernel void OrientationMatchingMetricSparseMask( 
                                        __constant REAL4 * rigidContext,
                                        __global REAL8* g_fg, __global REAL4* g_fl,
                                        read_only image3d_t mgImage,
                                        __global REAL * metricOutput, 
                                        __local REAL * metricAccums
                                        )
{

  unsigned int gidx = get_global_id(0);
  unsigned int lid = get_local_id(0);
  unsigned int groupID = get_group_id(0);                                      

  /* Evaluate Fixed Image Gradient */  
  REAL4 loc = g_fl[gidx];

  const sampler_t mySampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

  REAL4 rcX = rigidContext[0];
  REAL4 rcY = rigidContext[1];
  REAL4 rcZ = rigidContext[2];

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

  REAL8 fixedGradAndBasis = g_fg[gidx];  

  REAL2 fixedGrad = fixedGradAndBasis.s01;
  REAL4 base1 = (REAL4)(0.0f, 0.0f, 0.0f, 0.0f);
  base1.x = fixedGradAndBasis.s2;
  base1.y = fixedGradAndBasis.s3;
  base1.z = fixedGradAndBasis.s4;

  REAL4 base2 = (REAL4)(0.0f, 0.0f, 0.0f, 0.0f);
  base2.x = fixedGradAndBasis.s5;
  base2.y = fixedGradAndBasis.s6;
  base2.z = fixedGradAndBasis.s7;

  //REAL4 fixedGradN = normalize(fixedGrad);
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
#endif

#ifdef VERSION_C
__kernel void OrientationMatchingMetricSparseMask(
                                        __constant REAL4 * rigidContext,
                                        __global REAL2* g_fg, __global REAL4* g_fl,
                                        read_only image3d_t mgImage,
                                        __global REAL * metricOutput,
                                        __local REAL * metricAccums,
                                        __global REAL * globalGradientBase
                                        )
{

  __local REAL localGradientBase[SIZE_OF_GRADIENT_BASE];

  unsigned int gidx = get_global_id(0);
  unsigned int lid = get_local_id(0);
  unsigned int groupID = get_group_id(0);

  /* Evaluate Fixed Image Gradient */
  REAL4 loc = g_fl[gidx];

  event_t copydone = async_work_group_copy(localGradientBase, globalGradientBase, SIZE_OF_GRADIENT_BASE, 0);

  const sampler_t mySampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

  REAL4 rcX = rigidContext[0];
  REAL4 rcY = rigidContext[1];
  REAL4 rcZ = rigidContext[2];

  unsigned int sliceIndex = (unsigned int)(loc.w);
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

  wait_group_events(1, &copydone);

  REAL2 fixedGrad = g_fg[gidx];

  REAL4 base1 = (REAL4)(0.0f, 0.0f, 0.0f, 0.0f);
  base1.x = localGradientBase[sliceIndex*6 + 0];
  base1.y = localGradientBase[sliceIndex*6 + 1];
  base1.z = localGradientBase[sliceIndex*6 + 2];

  REAL4 base2 = (REAL4)(0.0f, 0.0f, 0.0f, 0.0f);
  base2.x = localGradientBase[sliceIndex*6 + 3];
  base2.y = localGradientBase[sliceIndex*6 + 4];
  base2.z = localGradientBase[sliceIndex*6 + 5];

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
#endif
