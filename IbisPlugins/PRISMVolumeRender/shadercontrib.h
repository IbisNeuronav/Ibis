#ifndef __ShaderContrib_h_
#define __ShaderContrib_h_

#include <QString>

struct ShaderContrib
{
    ShaderContrib() : custom( false ), name("NoName") {}
    ShaderContrib( const ShaderContrib & other )
        : custom(other.custom), name(other.name), code(other.code) {}
    bool operator==(const ShaderContrib & other) const
    {
        bool res = (this->custom == other.custom);
        res &= (this->name == other.name);
        res &= (this->code == other.code);
        return res;
    }
    bool operator!=( const ShaderContrib & other ) const
    {
        return !(*this == other);
    }
    bool custom;
    QString name;
    QString code;
};

#endif

