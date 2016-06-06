/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_STRINGTOOLS_H
#define TAG_STRINGTOOLS_H

void Tokenize( const std::string & str, std::vector<std::string> & tokens, const std::string & delimiters = " \r" );
void TokenizeRemovingQuotes( const std::string & str, std::vector<std::string> & tokens, const std::string & delimiters = " \r" );

#endif //TAG_STRINGTOOLS_H
