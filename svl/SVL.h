/*
    File:           SVL.h

    Function:       Master header for a simple version of the VL library.
                    The various classes are named Vec2, Mat3, Vec, etc.
                    Link with -lsvl, or define the symbol VL_DEBUG and
                    link with -lsvl.dbg for the debugging version.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott
 */


#ifndef __SVL__
#define __SVL__

#define SVL_VERSION "1.5"
#define SVL_VER_NUM 10500

#ifdef VL_DEBUG
#define VL_CHECKING
#endif

#include <iostream>

#include "Basics.h"
#include "Constants.h"
#include "Utils.h"

#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Vec.h"

#include "Mat2.h"
#include "Mat3.h"
#include "Mat4.h"
#include "Mat.h"

#include "Transform.h"

#endif
