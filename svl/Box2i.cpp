#include "Box2i.h"

Box2i::Box2i( int xmin, int xmax, int ymin, int ymax )
    : m_xMin( xmin )
    , m_xMax( xmax )
    , m_yMin( ymin )
    , m_yMax( ymax )
{}

bool Box2i::IsInside( int x, int y ) const
{
    if( x >= m_xMin && x <= m_xMax && y >= m_yMin && y <= m_yMax )
        return true;
    return false;
}	

void Box2i::IncludePoint( int x, int y )
{
    if(x < m_xMin)
        m_xMin = x;
    else if( x > m_xMax )
        m_xMax = x;
    if( y < m_yMin )
        m_yMin = y;
    else if( y > m_yMax )
        m_yMax = y;
}	

bool Box2i::Intersect( const Box2i & other ) const
{
    if( IsInside( other.m_xMin, other.m_yMin ) ||
        IsInside( other.m_xMax, other.m_yMin ) ||
        IsInside( other.m_xMin, other.m_yMax ) ||
        IsInside( other.m_xMax, other.m_yMax ) )
        return true;
    return false;
}

bool Box2i::Intersect( const Box2i & b1, const Box2i & b2 )
{
    if( b1.Intersect( b2 ) )
        return true;
    return false;
}	
