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
#define DIM_3
#define INT uint

#ifdef DIM_3
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
