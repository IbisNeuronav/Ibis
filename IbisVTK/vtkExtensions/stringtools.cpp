/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <string>
#include <vector>
#include <sstream>
#include "stringtools.h"

//function Tokenize() is written by Simon Drouin
void Tokenize( const std::string & str, std::vector<std::string> & tokens, const std::string & delimiters )
{
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delimiters, lastPos);
    while ( std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

void TokenizeRemovingQuotes( const std::string & str, std::vector<std::string> & tokens, const std::string & delimiters )
{
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delimiters, lastPos);
    while ( std::string::npos != pos || std::string::npos != lastPos)
    {
        if (str[lastPos] == '\"')
        {
            ++lastPos;
            std::string::size_type pos1 = str.find_first_of('\"', lastPos);
            // Found a token, add it to the vector.
            tokens.push_back(str.substr(lastPos, pos1 - lastPos));
            pos = str.find_first_of(delimiters, pos1);
        }
        else // Found a token, add it to the vector.
            tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}
