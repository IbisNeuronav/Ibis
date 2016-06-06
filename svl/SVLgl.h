/*
    File:       SVLgl.h

    Purpose:    Provides some handy wrappers for using svl with
                OpenGL.
 */

#ifndef __SVL_GL__
#define __SVL_GL__

#ifdef VL_FLOAT

inline Void glVertex(const Vec2 &a)
{ glVertex2fv(a.Ref()); }

inline Void glVertex(const Vec3 &a)
{ glVertex3fv(a.Ref()); }

inline Void glVertex(const Vec4 &a)
{ glVertex4fv(a.Ref()); }

inline Void glColor(const Vec3 &a)
{ glColor3fv(a.Ref()); }

inline Void glColor(const Vec4 &a)
{ glColor4fv(a.Ref()); }

inline Void glNormal(const Vec3 &a)
{ glNormal3fv(a.Ref()); }

inline Void glTranslate( const Vec2 & a )
{ glTranslatef( a(0), a(1), 0 ); }

#else

inline Void glVertex(const Vec2 &a)
{ glVertex2dv(a.Ref()); }

inline Void glVertex(const Vec3 &a)
{ glVertex3dv(a.Ref()); }

inline Void glVertex(const Vec4 &a)
{ glVertex4dv(a.Ref()); }

inline Void glColor(const Vec3 &a)
{ glColor3dv(a.Ref()); }

inline Void glColor(const Vec4 &a)
{ glColor4dv(a.Ref()); }

inline Void glNormal(const Vec3 &a)
{ glNormal3dv(a.Ref()); }

inline Void glTranslate( const Vec2 & a )
{ glTranslated( a[0], a[1], 0 ); }


#endif

#endif
