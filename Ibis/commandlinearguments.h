/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef COMMANDLINEARGUMENTS_H
#define COMMANDLINEARGUMENTS_H

#include <QStringList>

class CommandLineArguments
{
public:
    CommandLineArguments();
    bool ParseArguments( QStringList & args );

    bool GetViewerOnly() { return m_viewerOnly; }
    bool GetLoadPrevConfig() { return m_loadPrevConfig; }
    bool GetLoadDefaultConfig() { return m_loadDefaultConfig; }
    bool GetLoadConfigFile() { return m_loadConfigFile; }
    QString GetConfigFile() { return m_configFile; }
    QStringList GetDataFilesToLoad() { return m_loadFileNames; }

protected:
    bool m_viewerOnly;
    bool m_loadPrevConfig;
    bool m_loadDefaultConfig;
    bool m_loadConfigFile;
    QString m_configFile;
    QStringList m_loadFileNames;
};

#endif
