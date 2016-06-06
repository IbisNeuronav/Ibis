/*
    File:           Vec2.cpp

    Function:       Implements Vec2.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott

*/


#include "Vec2.h"
#include <cctype>
#include <iomanip>


ostream &operator << (ostream &s, const Vec2 &v)
{
    Int w = s.width();

    return(s << '[' << v[0] << ' ' << setw(w) << v[1] << ']');
}

istream &operator >> (istream &s, Vec2 &v)
{
    Vec2    result;
    Char    c;

    // Expected format: [1 2]

    while (s >> c && isspace(c))
        ;

    if (c == '[')
    {
        s >> result[0] >> result[1];

        if (!s)
        {
            cerr << "Error: Expected number while reading vector\n";
            return(s);
        }

        while (s >> c && isspace(c))
            ;

        if (c != ']')
        {
            s.clear(ios::failbit);
            cerr << "Error: Expected ']' while reading vector\n";
            return(s);
        }
    }
    else
    {
        s.clear(ios::failbit);
        cerr << "Error: Expected '[' while reading vector\n";
        return(s);
    }

    v = result;
    return(s);
}

