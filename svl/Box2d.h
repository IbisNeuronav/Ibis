#ifndef __Box2d_h_
#define __Box2d_h_

class Box2d
{

public:

    Box2d();
    Box2d( double xmin, double xmax, double ymin, double ymax );

    void Init();
    void Init( double xmin, double xmax, double ymin, double ymax );

    //  determine if (x,y) is inside the box
    bool IsInside( double x, double y ) const;

    //  ajust box (if needed) so that (x,y) is inside
    void IncludePoint( double x, double y );

    double GetXMin() { return m_xMin; }
    double GetYMin() { return m_yMin; }
    double GetXMax() { return m_xMax; }
    double GetYMax() { return m_yMax; }
    double GetWidth() { return m_xMax - m_xMin; }
    double GetHeight() { return m_yMax - m_yMin; }

    bool Intersect( const Box2d & other ) const;
    static bool Intersect( const Box2d & b1, const Box2d & b2 );

private:

    double m_xMin;
    double m_xMax;
    double m_yMin;
    double m_yMax;

};

#endif
