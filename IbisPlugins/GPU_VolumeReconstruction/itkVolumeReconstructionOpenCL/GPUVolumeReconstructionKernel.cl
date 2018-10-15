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


__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
 

__kernel void VolumeReconstructionPopulating(
                                          __read_only image3d_t inputSlices,
                                         //__global unsigned char* sliceMask,
                                         __read_only image2d_t sliceMask,
                                         __global REAL2* outputWeightAndWeightedValue,
                                         int sliceWidth, int sliceHeight, int nbrOfInputSlices,
                                         __global REAL4* allMatrices,
                                         __local REAL4 * localBuffer, 
                                         int sliceCntr, 
                                         int outWidth, int outHeight, int outDepth, int usSearchRadius, 
                                         REAL stdDev
                                        )
{
  int gix = get_global_id(0);
  int giy = get_global_id(1);
  int giz = get_global_id(2);

  event_t copydone1 = async_work_group_copy(localBuffer, allMatrices, 3*(2*nbrOfInputSlices+1), 0);

  unsigned int gidx = outWidth*(giz*outHeight + giy) + gix;

  bool isValid = true;
  if(gix < 0 || gix >= outWidth) isValid = false;
  if(giy < 0 || giy >= outHeight) isValid = false;
  if(giz < 0 || giz >= outDepth) isValid = false;

  REAL4 volumeIndex;
  volumeIndex.x = gix;
  volumeIndex.y = giy;
  volumeIndex.z = giz;
  volumeIndex.w = 1.0f;

  REAL variance = stdDev * stdDev;

  bool updatedValue = false;

  if(isValid)
  {

    wait_group_events(1, &copydone1);

    REAL2 accumWeightandWeightedValue = outputWeightAndWeightedValue[gidx];
    REAL4 volumeLocation;
    volumeLocation.x = dot(localBuffer[0], volumeIndex);
    volumeLocation.y = dot(localBuffer[1], volumeIndex);
    volumeLocation.z = dot(localBuffer[2], volumeIndex);
    volumeLocation.w = 0.0f;  

    for(int sliceIdx=0; sliceIdx < nbrOfInputSlices; sliceIdx++)
    {
      REAL4 sliceIndex;
      sliceIndex.x = dot(localBuffer[3*(sliceIdx+1)+0], volumeIndex);
      sliceIndex.y = dot(localBuffer[3*(sliceIdx+1)+1], volumeIndex);
      sliceIndex.z = dot(localBuffer[3*(sliceIdx+1)+2], volumeIndex);
      sliceIndex.w = 1.0f;

      REAL4 roundedSliceIndex = round(sliceIndex);

      bool sliceIndexIsValid = true;
      if(roundedSliceIndex.x < 0 || roundedSliceIndex.x >= sliceWidth) sliceIndexIsValid = false;
      if(roundedSliceIndex.y < 0 || roundedSliceIndex.y >= sliceHeight) sliceIndexIsValid = false;

      unsigned char maskValue = 0;
      if(sliceIndexIsValid)
      {             
        int2 maskCoord;  
        maskCoord.x = (int)roundedSliceIndex.x;
        maskCoord.y = (int)roundedSliceIndex.y;
        maskValue = read_imageui(sliceMask, sampler, maskCoord).x;
      }
  
     if(maskValue > 0)
     {                
       REAL4 currentSliceIndex;
       int4 coord;
       for(int iy = max(0, (int)roundedSliceIndex.y - usSearchRadius); iy <= min(sliceHeight, (int)roundedSliceIndex.y + usSearchRadius); iy++)
       {
         currentSliceIndex.y = (REAL)iy;
         coord.y = iy;
         for(int ix = max(0, (int)roundedSliceIndex.x - usSearchRadius); ix <= min(sliceWidth, (int)roundedSliceIndex.x + usSearchRadius); ix++)
         {
           currentSliceIndex.x = (REAL)ix;                                
           currentSliceIndex.z = 0.0f;
           currentSliceIndex.w = 1.0f;

           coord.x = ix;
           coord.z = sliceIdx;
           coord.w = 0;

           //Evaluate Physical Location
           REAL4 sliceLocation;
           sliceLocation.x = dot(localBuffer[3*(sliceIdx+1+nbrOfInputSlices) + 0], currentSliceIndex);
           sliceLocation.y = dot(localBuffer[3*(sliceIdx+1+nbrOfInputSlices) + 1], currentSliceIndex);
           sliceLocation.z = dot(localBuffer[3*(sliceIdx+1+nbrOfInputSlices) + 2], currentSliceIndex);
           sliceLocation.w = 0.0f;            

           REAL dist = distance(volumeLocation, sliceLocation);

           if(dist < 1.0f)
           {
            int2 maskCoord; 
            maskCoord.x = (int)ix;
            maskCoord.y = (int)iy;
            maskValue = read_imageui(sliceMask, sampler, maskCoord).x;
            if(maskValue > 0)
            {     
             REAL usIntensity = read_imagef(inputSlices, sampler, coord).x;
             REAL currentWeight = exp( -(dist*dist)/( 2.0f * variance ) );          
             REAL currentWeightedValue = currentWeight * usIntensity;             
             accumWeightandWeightedValue.x += currentWeightedValue;                       
             accumWeightandWeightedValue.y += currentWeight; 
             updatedValue = true;
            }
           }
         }       
       }
      }
    }
    if(updatedValue)
      outputWeightAndWeightedValue[gidx] = accumWeightandWeightedValue;  
  }
}
