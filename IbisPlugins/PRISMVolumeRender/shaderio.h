#ifndef __ShaderIO_h_
#define __ShaderIO_h_

#include "shadercontrib.h"
#include <QList>

class ShaderIO
{

public:

    ShaderIO() {}
    ~ShaderIO() {}

    void LoadShaders( QString baseDir );
    void SaveShaders( QString baseDir );

    QList<ShaderContrib> & GetInitShaders() { return m_initShaders; }
    void SetInitShaders( const QList<ShaderContrib> & shaders ) { m_initShaders = shaders; }
    QList<ShaderContrib> & GetVolumeShaders() { return m_volumeShaders; }
    void SetVolumeShaders( const QList<ShaderContrib> & shaders ) { m_volumeShaders = shaders; }
    QList<ShaderContrib> & GetStopConditionShaders() { return m_stopConditionShaders; }
    void SetStopConditionShaders( const QList<ShaderContrib> & shaders ) { m_stopConditionShaders = shaders; }

protected:

    void LoadShaderDir( QString dir, QList<ShaderContrib> & shaders );
    void SaveShaderDir( QString dir, QList<ShaderContrib> & shaders );

    QList<ShaderContrib> m_initShaders;
    QList<ShaderContrib> m_volumeShaders;
    QList<ShaderContrib> m_stopConditionShaders;
};

#endif
