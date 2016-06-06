/*
    File:           Transform.h

    Function:       Provides transformation constructors.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott
*/

#ifndef __SVL_TRANSFORM__
#define __SVL_TRANSFORM__

inline Mat2 Rot2(Real theta)
            { Mat2 result; result.MakeRot(theta); return(result); }
inline Mat2 Scale2(const Vec2 &s)
            { Mat2 result; result.MakeScale(s); return(result); }

inline Mat3 Rot3(const Vec3 &axis, Real theta)
            { Mat3 result; result.MakeRot(axis, theta); return(result); }
inline Mat3 Rot3(const Vec4 &q)
            { Mat3 result; result.MakeRot(q); return(result); }
inline Mat3 Scale3(const Vec3 &s)
            { Mat3 result; result.MakeScale(s); return(result); }
inline Mat3 HRot3(Real theta)
            { Mat3 result; result.MakeHRot(theta); return(result); }
inline Mat3 HScale3(const Vec2 &s)
            { Mat3 result; result.MakeHScale(s); return(result); }
inline Mat3 HTrans3(const Vec2 &t)
            { Mat3 result; result.MakeHTrans(t); return(result); }

inline Mat4 HRot4(const Vec3 &axis, Real theta)
            { Mat4 result; result.MakeHRot(axis, theta); return(result); }
inline Mat4 HRot4(const Vec4 &q)
            { Mat4 result; result.MakeHRot(q); return(result); }
inline Mat4 HScale4(const Vec3 &s)
            { Mat4 result; result.MakeHScale(s); return(result); }
inline Mat4 HTrans4(const Vec3 &t)
            { Mat4 result; result.MakeHTrans(t); return(result); }

#endif
