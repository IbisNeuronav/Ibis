#ifndef __ShaderContrib_h_
#define __ShaderContrib_h_

#include <QString>

struct ShaderContrib
{
    ShaderContrib() : custom( false ), name("NoName") {}
    ShaderContrib( const ShaderContrib & other ) : custom(other.custom), name(other.name), code(other.code) {}
    bool custom;
    QString name;
    QString code;
};

#endif

