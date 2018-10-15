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

#define INTYPE float
#define OUTTYPE float
#define OPTYPE float
#define OUTTYPE4 float4
#define OUTTYPE2 float2

#ifdef DIM_3
__kernel void SeparableNeighborOperatorFilter(const __global INTYPE* in,
                                     __global OUTTYPE4* out,
                                     __constant OPTYPE* opx, __constant OPTYPE* opy, __constant OPTYPE* opz,
                                     int radiusx, int radiusy, int radiusz,
                                     int width, int height, int depth,
                                     OPTYPE spx, OPTYPE spy, OPTYPE spz)
{
  int gix = get_global_id(0);
  int giy = get_global_id(1);
  int giz = get_global_id(2);
  unsigned int gidx = width*(giz*height + giy) + gix;
  OPTYPE sumx = 0;
  OPTYPE sumy = 0;
  OPTYPE sumz = 0;
  unsigned int opIdx = 0;

  bool isValid = true;
  if(gix < 0 || gix >= width) isValid = false;
  if(giy < 0 || giy >= height) isValid = false;
  if(giz < 0 || giz >= depth) isValid = false;


  if( isValid )
  {
    float4 gradient = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    for(int x = gix-radiusx; x <= gix+radiusx; x++)
    {
      unsigned int cidx = width*(giz*height + giy) + (unsigned int)(min(max(0, x),width-1));
      sumx += (OPTYPE)in[cidx] * (OPTYPE)opx[opIdx];
      opIdx++;
    }
    gradient.x = (OUTTYPE)(sumx/spx);

    opIdx = 0;
    for(int y = giy-radiusy; y <= giy+radiusy; y++)
    {
      unsigned int yid = (unsigned int)(min(max(0, y),height-1));
      unsigned int cidx = width*(giz*height + yid) + gix;
      sumy += (OPTYPE)in[cidx] * (OPTYPE)opy[opIdx];
      opIdx++;
    }
    gradient.y = (OUTTYPE)(sumy/spy);


    opIdx = 0;
    for(int z = giz-radiusz; z <= giz+radiusz; z++)
    {
      unsigned int zid = (unsigned int)(min(max(0, z),depth-1));
      unsigned int cidx = width*(zid*height + giy) + gix;
      sumz += (OPTYPE)in[cidx] * (OPTYPE)opz[opIdx];
      opIdx++;
    }
    gradient.z = (OUTTYPE)(sumz/spz);

    out[gidx] = gradient;

  }
}

__kernel void SeparableNeighborOperatorFilterThresholder(const __global INTYPE* in,
                                     __global OUTTYPE4* out,
                                     __constant OPTYPE* opx, __constant OPTYPE* opy, __constant OPTYPE* opz,
                                     int radiusx, int radiusy, int radiusz,
                                     int width, int height, int depth,
                                     OPTYPE spx, OPTYPE spy, OPTYPE spz )
{
  int gix = get_global_id(0);
  int giy = get_global_id(1);
  int giz = get_global_id(2);
  unsigned int gidx = width*(giz*height + giy) + gix;
  OPTYPE sumx = 0;
  OPTYPE sumy = 0;
  OPTYPE sumz = 0;
  OUTTYPE maskValue = (OUTTYPE)-1.0f;
  bool maskBool = true;
  unsigned int opIdx = 0;

  bool isValid = true;
  if(gix < 0 || gix >= width) isValid = false;
  if(giy < 0 || giy >= height) isValid = false;
  if(giz < 0 || giz >= depth) isValid = false;

  if( isValid )
  {
    float4 gradient = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    for(int x = gix-radiusx; x <= gix+radiusx; x++)
    {
      unsigned int cidx = width*(giz*height + giy) + (unsigned int)(min(max(0, x),width-1));
      sumx += (OPTYPE)in[cidx] * (OPTYPE)opx[opIdx];
      if(in[cidx] < 0.001f) maskBool = false;
      opIdx++;
    }
    gradient.x = (OUTTYPE)(sumx/spx);


    opIdx = 0;
    for(int y = giy-radiusy; y <= giy+radiusy; y++)
    {
      unsigned int yid = (unsigned int)(min(max(0, y),height-1));
      unsigned int cidx = width*(giz*height + yid) + gix;
      sumy += (OPTYPE)in[cidx] * (OPTYPE)opy[opIdx];
      if(in[cidx] < 0.001f) maskBool = false;
      opIdx++;
    }
    gradient.y = (OUTTYPE)(sumy/spy);


    opIdx = 0;
    for(int z = giz-radiusz; z <= giz+radiusz; z++)
    {
      unsigned int zid = (unsigned int)(min(max(0, z),depth-1));
      unsigned int cidx = width*(zid*height + giy) + gix;
      sumz += (OPTYPE)in[cidx] * (OPTYPE)opz[opIdx];
      if(in[cidx] < 0.001f) maskBool = false;
      opIdx++;
    }
    gradient.z = (OUTTYPE)(sumz/spz);

    if(maskBool == true) gradient.w = 1.0f;

    out[gidx] = gradient;

  }
}
#endif

#ifdef DIM_2
__kernel void SeparableNeighborOperatorFilter(const __global INTYPE* in,
                                     __global OUTTYPE2* out,
                                     __constant OPTYPE* opx, __constant OPTYPE* opy,
                                     int radiusx, int radiusy, 
                                     int width, int height,
                                     OPTYPE spx, OPTYPE spy)
{
  int gix = get_global_id(0);
  int giy = get_global_id(1);

  unsigned int gidx = width*giy + gix;
  OPTYPE sumx = 0;
  OPTYPE sumy = 0;
  unsigned int opIdx = 0;

  bool isValid = true;
  if(gix < 0 || gix >= width) isValid = false;
  if(giy < 0 || giy >= height) isValid = false;


  if( isValid )
  {
    OUTTYPE2 gradient = (OUTTYPE2)(0.0f, 0.0f);
    for(int x = gix-radiusx; x <= gix+radiusx; x++)
    {
      unsigned int cidx = width*giy  + (unsigned int)(min(max(0, x),width-1));
      sumx += (OPTYPE)in[cidx] * (OPTYPE)opx[opIdx];
      opIdx++;
    }
    gradient.x = (OUTTYPE)(sumx/spx);

    opIdx = 0;
    for(int y = giy-radiusy; y <= giy+radiusy; y++)
    {
      unsigned int yid = (unsigned int)(min(max(0, y),height-1));
      unsigned int cidx = width*yid + gix;
      sumy += (OPTYPE)in[cidx] * (OPTYPE)opy[opIdx];
      opIdx++;
    }
    gradient.y = (OUTTYPE)(sumy/spy);


    out[gidx] = gradient;

  }
}
#endif

#ifdef DIM_2_ALL
__kernel void SeparableNeighborOperatorFilter(const __global INTYPE* in,
                                     __global OUTTYPE2* out,
                                     __constant OPTYPE* opx, __constant OPTYPE* opy,
                                     int radiusx, int radiusy,
                                     int width, int height, int depth,
                                     OPTYPE spx, OPTYPE spy)
{
  int gix = get_global_id(0);
  int giy = get_global_id(1);
  int giz = get_global_id(2);

  unsigned int gidx = width*(giz*height + giy) + gix;
  OPTYPE sumx = 0;
  OPTYPE sumy = 0;
  unsigned int opIdx = 0;

  bool isValid = true;
  if(gix < 0 || gix >= width) isValid = false;
  if(giy < 0 || giy >= height) isValid = false;
  if(giz < 0 || giz >= depth) isValid = false;


  if( isValid )
  {
    OUTTYPE2 gradient = (OUTTYPE2)(0.0f, 0.0f);
    for(int x = gix-radiusx; x <= gix+radiusx; x++)
    {
      unsigned int cidx = width*giz*height + width*giy  + (unsigned int)(min(max(0, x),width-1));
      sumx += (OPTYPE)in[cidx] * (OPTYPE)opx[opIdx];
      opIdx++;
    }
    gradient.x = (OUTTYPE)(sumx/spx);

    opIdx = 0;
    for(int y = giy-radiusy; y <= giy+radiusy; y++)
    {
      unsigned int yid = (unsigned int)(min(max(0, y),height-1));
      unsigned int cidx = width*giz*height + width*yid + gix;
      sumy += (OPTYPE)in[cidx] * (OPTYPE)opy[opIdx];
      opIdx++;
    }
    gradient.y = (OUTTYPE)(sumy/spy);

    out[gidx] = gradient;
  }
}
#endif
