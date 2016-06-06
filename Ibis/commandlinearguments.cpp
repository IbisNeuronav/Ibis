/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "commandlinearguments.h"
#include <iostream>

CommandLineArguments::CommandLineArguments()
    : m_viewerOnly( false )
    , m_loadPrevConfig( false )
    , m_loadDefaultConfig( false )
    , m_loadConfigFile( false )
{
}

bool CommandLineArguments::ParseArguments( QStringList & args )
{
    for( int i = 1; i < args.size(); ++i )
    {
        QString arg = args[i];
        if( arg == "-l")
            m_loadPrevConfig = true;
        else if( arg == "-d")
            m_loadDefaultConfig = true;
        else if( arg == "-f" )
        {
            m_loadConfigFile = true;
            if( args.size() > i + 1 )
            {
                QString nextArg = args[i+1];
                if( nextArg.startsWith('-') )
                {
                    std::cerr << "Error: expecting config filename after -f option" << std::endl;
                    return false;
                }
                m_configFile = nextArg;
                ++i;
            }
            else
            {
                std::cerr << "Error: expecting config filename after -f option" << std::endl;
                return false;
            }
        }
        else if( arg == "-v")
            m_viewerOnly = true;
        else
            m_loadFileNames.push_back( arg );
    }
    return true;
}
