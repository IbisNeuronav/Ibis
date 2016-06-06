#include "Box2d.h"
#include <limits>

Box2d::Box2d()
    : m_xMin( std::numeric_limits<double>::max() )
    , m_xMax( std::numeric_limits<double>::min() )
    , m_yMin( std::numeric_limits<double>::max() )
    , m_yMax( std::numeric_limits<double>::min() )
{
}

Box2d::Box2d( double xmin, double xmax, double ymin, double ymax )
    : m_xMin( xmin )
    , m_xMax( xmax )
    , m_yMin( ymin )
    , m_yMax( ymax )
{}

void Box2d::Init()
{
    m_xMin = std::numeric_limits<double>::max();
    m_xMax = std::numeric_limits<double>::min();
    m_yMin = std::numeric_limits<double>::max();
    m_yMax = std::numeric_limits<double>::min();
}

void Box2d::Init( double xmin, double xmax, double ymin, double ymax )
{
    m_xMin = xmin;
    m_yMin = ymin;
    m_xMax = xmax;
    m_yMax = ymax;
}

bool Box2d::IsInside( double x, double y ) const
{
    if( x >= m_xMin && x <= m_xMax && y >= m_yMin && y <= m_yMax )
        return true;
    return false;
}	

void Box2d::IncludePoint( double x, double y )
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

bool Box2d::Intersect( const Box2d & other ) const
{
    if( IsInside( other.m_xMin, other.m_yMin ) ||
        IsInside( other.m_xMax, other.m_yMin ) ||
        IsInside( other.m_xMin, other.m_yMax ) ||
        IsInside( other.m_xMax, other.m_yMax ) )
        return true;
    return false;
}

bool Box2d::Intersect( const Box2d & b1, const Box2d & b2 )
{
    if( b1.Intersect( b2 ) )
        return true;
    return false;
}	
