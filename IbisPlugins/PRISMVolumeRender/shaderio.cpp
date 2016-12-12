#include "shaderio.h"
#include <QDir>
#include <QTextStream>

#define INIT_SHADERS_DIR "PRISM/InitShaders/"
#define VOLUME_SHADERS_DIR "PRISM/VolumeShaders/"
#define STOP_CONDITION_SHADERS_DIR "PRISM/StopConditionShaders/"

void ShaderIO::LoadShaders( QString baseDir )
{
    QString initShaderDir = baseDir + INIT_SHADERS_DIR;
    LoadShaderDir( initShaderDir, m_initShaders );
    QString volumeShaderDir = baseDir + VOLUME_SHADERS_DIR;
    LoadShaderDir( volumeShaderDir, m_volumeShaders );
    QString stopConditionShaderDir = baseDir + STOP_CONDITION_SHADERS_DIR;
    LoadShaderDir( stopConditionShaderDir, m_stopConditionShaders );
}

void ShaderIO::SaveShaders( QString baseDir )
{
    QString initShaderDir = baseDir + INIT_SHADERS_DIR;
    SaveShaderDir( initShaderDir, m_initShaders );
    QString volumeShaderDir = baseDir + VOLUME_SHADERS_DIR;
    SaveShaderDir( volumeShaderDir, m_volumeShaders );
    QString stopConditionShaderDir = baseDir + STOP_CONDITION_SHADERS_DIR;
    SaveShaderDir( stopConditionShaderDir, m_stopConditionShaders );
}

void ShaderIO::LoadShaderDir( QString dirName, QList<ShaderContrib> & shaders )
{
    if( QFile::exists( dirName ) )
    {
        // list all .glsl files
        QDir dir( dirName );
        QStringList filter;
        filter.push_back( QString("*.glsl") );
        QStringList allFiles = dir.entryList( filter );

        // for each file, read and create a shader contrib.
        for( int i = 0; i < allFiles.size(); ++i )
        {
            QString shaderFileName = dirName + allFiles[i];
            QFile shaderFile( shaderFileName );
            if( shaderFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
            {
                ShaderContrib contrib;
                contrib.custom = true;
                QFileInfo info( allFiles[i] );
                contrib.name = info.baseName();
                QTextStream shaderStream( &shaderFile );
                contrib.code = shaderStream.readAll();
                shaders.push_back( contrib );
            }
            shaderFile.close();
        }
    }
}

void ShaderIO::SaveShaderDir( QString dirName, QList<ShaderContrib> & shaders )
{
    // Make sure dir exists
    if( !QFile::exists( dirName ) )
    {
        QDir dir;
        dir.mkpath( dirName );
    }

    for( int i = 0; i < shaders.size(); ++i )
    {
        if( shaders[i].custom )
        {
            QString filename = dirName + shaders[i].name + ".glsl";
            QFile shaderFile( filename );
            if( shaderFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
            {
                QTextStream shaderStream( &shaderFile );
                shaderStream << shaders[i].code;
            }
            shaderFile.close();
        }
    }
}
