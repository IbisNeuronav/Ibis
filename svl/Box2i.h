#ifndef __Box2i_h_
#define __Box2i_h_

class Box2i
{

public:

    Box2i( int xmin, int xmax, int ymin, int ymax );

    //  determine if (x,y) is inside the box
    bool IsInside( int x, int y ) const;

    //  ajust box (if needed) so that (x,y) is inside
    void IncludePoint( int x, int y );

    int GetXOrigin() { return m_xMin; }
    int GetYOrigin() { return m_yMin; }
    int GetWidth() { return m_xMax - m_xMin; }
    int GetHeight() { return m_yMax - m_yMin; }

    bool Intersect( const Box2i & other ) const;
    static bool Intersect( const Box2i & b1, const Box2i & b2 );

private:

    int m_xMin;
    int m_xMax;
    int m_yMin;
    int m_yMax;

};

#endif
