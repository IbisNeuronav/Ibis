/*
    File:           Basics.cpp

    Function:       Implements Basics.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott

    Notes:          

*/

#include "Basics.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>


using namespace std;


// --- Error functions for range and routine checking -------------------------


static Void DebuggerBreak()
{
    abort();
}

Void _Assert(Int condition, const Char *errorMessage, const Char *file, Int line)
{
    if (!condition)
    {
        Char reply;
        
        cerr << "\n*** Assert failed (line " << line << " in " << 
            file << "): " << errorMessage << endl;
        cerr << "    Continue? [y/n] ";
        cin >> reply;
        
        if (reply != 'y')
        {
            DebuggerBreak();
            exit(1);
        }
    }
}

Void _Expect(Int condition, const Char *warningMessage, const Char *file, Int line)
{
    if (!condition)
        cerr << "\n*** Warning (line " << line << " in " << file << "): " <<
            warningMessage << endl;
}

Void _CheckRange(Int i, Int lowerBound, Int upperBound, 
                 const Char *rangeMessage, const Char *file, Int line)
{
    if (i < lowerBound || i >= upperBound)
    {
        Char reply;
        
        cerr << "\n*** Range Error (line " << line << " in " << file <<
            "): " << rangeMessage << endl;  
        cerr << "    Continue? [y/n] ";
        cin >> reply;
        
        if (reply != 'y')
        {
            DebuggerBreak();
            exit(1);
        }
    }
}
