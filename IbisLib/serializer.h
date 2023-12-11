/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __Serializer_h
#define __Serializer_h

#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>
#include <map>
#include <utility>
#include <vector>

/**
 * @class   Serializer
 * @brief   Base class for reading/writing xml files to save the settings of Ibis
 *
 * Typically, to write a file, you create a
 *
 * SerializerWriter writer;
 *
 * writer.SetFilename("afile.xml");\n
 * writer.Start();\n
 * Serialize( &writer, "attributename", attributeValue );\n
 * ...\n
 * writer.BeginSection();\n
 * ...\n
 * Serialize( &writer, "anotherattributename", anotherattributeValue );\n
 * ...\n
 * writer.EndSection();\n
 * ...\n
 * Serialize( &writer, "anotherattributename", anotherattributeValue );\n
 * writer.Finish()\n
 *
 * and you do the same to read a file. The Serializer class and its children
 * where made so that we use the same code for reading and writing. The only
 * thing that changes is the instanciated Serializer (SerializerWriter to write
 * and SerializerReader to read). In some cases, if you have to execute different
 * code in reading and writing cases, you can call the function IsReader() to
 * know which one you are in.
 *
 * See serializerhelper.cpp/.h to find definitions of functions used to serialize vtk classes or
 * classes from third party libs that are often used in Ibis:
 *
 * bool Serialize( Serializer * serial, const char * attrName, vtkGenericParamValues & value );\n
 * bool Serialize( Serializer * serial, const char * attrName, vtkColorTransferFunction * colorFunc );\n
 * bool Serialize( Serializer * serial, const char * attrName, vtkPiecewiseFunction * opacityFunc );\n
 * bool Serialize( Serializer * serial, const char * attrName, Vec3 & v );\n
 * bool Serialize( Serializer * serial, const char * attrName, vtkMatrix4x4 * mat );\n
 *
 * @sa SerializerReader SerializerWriter
 */
class Serializer
{
public:
    Serializer() : m_document( "configML" )
    {
        m_root = m_document.createElement( "configuration" );
        m_document.appendChild( m_root );
        m_currentNode = m_root;
    }
    virtual ~Serializer() {}

    /** Check if the currently used serializer is a reader. */
    virtual int IsReader() = 0;
    /** Set the name of the file to read/write. */
    void SetFilename( const char * name ) { m_filename = name; }
    /** Get the directory of the currently read/written file. */
    QString GetSerializationDirectory()
    {
        QFileInfo info( QString( m_filename.c_str() ) );
        return info.path();
    }
    /** Find the version of Serializer which created currently read file. */
    void ReadVersionFromFile()
    {
        Q_ASSERT( IsReader() );
        Serialize( "Version", m_versionFromFile );
    }
    /** Set currently supported version of Serializer, scene with a different versions may load incorrectly or not load
     * at all. */
    void SetSupportedVersion( QString version ) { m_supportedVersion = version; }
    /** Return Serializer version found in the currently read file. */
    QString GetVersionFromFile() { return m_versionFromFile; }
    /** Find if Serializer version found in the currently read file is too old. */
    bool FileVersionOlderThanSupported() { return FileVersionIsLowerThan( m_supportedVersion ); }
    /** Find if Serializer version used to create currently read file is newer than supported. */
    bool FileVersionNewerThanSupported() { return QString::compare( m_versionFromFile, this->m_supportedVersion ) > 0; }
    /** Find if Serializer version used to create currently read file is older than some other version. */
    bool FileVersionIsLowerThan( QString version ) { return QString::compare( m_versionFromFile, version ) < 0; }

    /** @name Reading and writing functions, defined respectively in SerializerReader and SerializerWriter
     */
    ///@{
    /** Start() has to be called before any other function to initialize serializer. */
    virtual bool Start() = 0;
    /** Finish() has to be called after all work is done to clean up. */
    virtual bool Finish() = 0;
    /** Open a section for writing/reading. */
    virtual bool BeginSection( const char * attrName ) = 0;
    /** Close section. */
    virtual void EndSection() = 0;

    /** Serialize an integer number. */
    virtual bool Serialize( const char * attrName, int & value ) = 0;
    /** Serialize a boolean. */
    virtual bool Serialize( const char * attrName, bool & value ) = 0;
    /** Serialize a double. */
    virtual bool Serialize( const char * attrName, double & value ) = 0;
    /** Serialize a standard string. */
    virtual bool Serialize( const char * attrName, std::string & value ) = 0;
    /** Serialize a Qstring. */
    virtual bool Serialize( const char * attrName, QString & value ) = 0;
    /** Serialize a nbElements of integers */
    virtual bool Serialize( const char * attrName, int * value, int nbElements ) = 0;
    /** Serialize a nbElements of doubles */
    virtual bool Serialize( const char * attrName, double * value, int nbElements ) = 0;
    ///@}

protected:
    std::string m_filename;
    QString m_versionFromFile;
    QString m_supportedVersion;
    QDomDocument m_document;
    QDomElement m_root;
    QDomNode m_currentNode;
};

/**
 * @class   SerializerWriter
 * @brief   Write xml files to store the parameters of scenes, objects etc.
 *
 **/
class SerializerWriter : public Serializer
{
public:
    SerializerWriter() {}
    ~SerializerWriter() {}

    virtual int IsReader() override { return 0; } 

    virtual bool Start() override
    {
        m_currentNode = m_root;
        return true;
    }

    virtual bool Finish() override
    {
        QFile file( m_filename.c_str() );
        if( !file.open( QIODevice::WriteOnly ) ) return false;

        QTextStream ts( &file );
        ts << m_document.toString();

        file.close();
        return true;
    }

    virtual bool BeginSection( const char * attrName ) override
    {
        QDomElement elem = m_document.createElement( attrName );
        m_currentNode    = m_currentNode.appendChild( elem );
        return true;
    }

    virtual void EndSection() override { m_currentNode = m_currentNode.parentNode(); }

    virtual bool Serialize( const char * attrName, int & value ) override
    {
        QDomElement elem = m_document.createElement( attrName );
        elem.setAttribute( "value", QString::number( value ) );
        m_currentNode.appendChild( elem );
        return true;
    }

    virtual bool Serialize( const char * attrName, bool & value ) override
    {
        QDomElement elem = m_document.createElement( attrName );
        elem.setAttribute( "value", QString::number( value ? 1 : 0 ) );
        m_currentNode.appendChild( elem );
        return true;
    }

    virtual bool Serialize( const char * attrName, double & value ) override
    {
        QDomElement elem = m_document.createElement( attrName );
        elem.setAttribute( "value", QString::number( value, 'e', 16 ) );
        m_currentNode.appendChild( elem );
        return true;
    }

    virtual bool Serialize( const char * attrName, std::string & value ) override
    {
        QDomElement elem = m_document.createElement( attrName );
        elem.setAttribute( "value", value.c_str() );
        m_currentNode.appendChild( elem );
        return true;
    }

    virtual bool Serialize( const char * attrName, QString & value ) override
    {
        QDomElement elem = m_document.createElement( attrName );
        elem.setAttribute( "value", value );
        m_currentNode.appendChild( elem );
        return true;
    }

    virtual bool Serialize( const char * attrName, int * value, int nbElements ) override
    {
        QDomElement elem = m_document.createElement( attrName );
        QString text;
        for( int i = 0; i < nbElements; i++ )
        {
            text += QString::number( value[i] ) + " ";
        }
        elem.setAttribute( "value", text );
        m_currentNode.appendChild( elem );
        return true;
    }

    virtual bool Serialize( const char * attrName, double * value, int nbElements ) override
    {
        QDomElement elem = m_document.createElement( attrName );
        QString text;
        for( int i = 0; i < nbElements; i++ )
        {
            text += QString::number( value[i], 'e', 16 ) + " ";
        }
        elem.setAttribute( "value", text );
        m_currentNode.appendChild( elem );
        return true;
    }
};

/**
 * @class   SerializerReader
 * @brief   Read xml files to get the parameters of scenes, objects etc.
 *
 **/
class SerializerReader : public Serializer
{
public:
    SerializerReader() {}
    ~SerializerReader() {}

    virtual int IsReader() override { return 1; }

    virtual bool Start() override
    {
        QFile file( m_filename.c_str() );

        if( !file.open( QIODevice::ReadOnly ) ) return false;

        if( !m_document.setContent( &file ) )
        {
            file.close();
            return false;
        }
        file.close();

        m_root = m_document.documentElement();
        if( m_root.tagName() != "configuration" )
        {
            // manage error!!!
            return false;
        }
        m_currentNode = m_root;
        return true;
    }

    virtual bool Finish() override { return true; }

    virtual bool BeginSection( const char * attrName ) override
    {
        QDomNode variableNode = m_currentNode.namedItem( attrName );
        if( !variableNode.isNull() )
        {
            m_currentNode = variableNode;
            return true;
        }
        return false;
    }

    virtual void EndSection() override { m_currentNode = m_currentNode.parentNode(); }

    virtual bool Serialize( const char * attrName, int & value ) override
    {
        QDomNode variableNode = m_currentNode.namedItem( attrName );
        if( !variableNode.isNull() )
        {
            QString variableValueString = variableNode.toElement().attribute( "value" );
            if( !variableValueString.isNull() )
            {
                value = variableValueString.toInt();
                return true;
            }
        }
        return false;
    }

    virtual bool Serialize( const char * attrName, bool & value ) override
    {
        QDomNode variableNode = m_currentNode.namedItem( attrName );
        if( !variableNode.isNull() )
        {
            QString variableValueString = variableNode.toElement().attribute( "value" );
            if( !variableValueString.isNull() )
            {
                value = variableValueString.toInt() == 0 ? false : true;
                return true;
            }
        }
        return false;
    }

    virtual bool Serialize( const char * attrName, double & value ) override
    {
        QDomNode variableNode = m_currentNode.namedItem( attrName );
        if( !variableNode.isNull() )
        {
            QString variableValueString = variableNode.toElement().attribute( "value" );
            if( !variableValueString.isNull() )
            {
                value = variableValueString.toDouble();
                return true;
            }
        }
        return false;
    }

    virtual bool Serialize( const char * attrName, std::string & value ) override
    {
        QDomNode variableNode = m_currentNode.namedItem( attrName );
        if( !variableNode.isNull() )
        {
            QString variableValueString = variableNode.toElement().attribute( "value" );
            if( !variableValueString.isNull() )
            {
                value = variableValueString.toUtf8().data();
                return true;
            }
        }
        return false;
    }

    virtual bool Serialize( const char * attrName, QString & value ) override
    {
        QDomNode variableNode = m_currentNode.namedItem( attrName );
        if( !variableNode.isNull() )
        {
            QString variableValueString = variableNode.toElement().attribute( "value" );
            if( !variableValueString.isNull() )
            {
                value = variableValueString.toUtf8().data();
                return true;
            }
        }
        return false;
    }

    virtual bool Serialize( const char * attrName, int * value, int nbElements ) override
    {
        QDomNode variableNode = m_currentNode.namedItem( attrName );
        if( !variableNode.isNull() )
        {
            QString variableValueString = variableNode.toElement().attribute( "value" );
            if( !variableValueString.isNull() )
            {
                QStringList list            = variableValueString.split( ' ' );
                QStringList::iterator it    = list.begin();
                QStringList::iterator itEnd = list.end();
                int i                       = 0;
                while( it != itEnd && i < nbElements )
                {
                    value[i] = ( *it ).toInt();
                    ++it;
                    ++i;
                }
                return true;
            }
        }
        return false;
    }

    virtual bool Serialize( const char * attrName, double * value, int nbElements ) override
    {
        QDomNode variableNode = m_currentNode.namedItem( attrName );
        if( !variableNode.isNull() )
        {
            QString variableValueString = variableNode.toElement().attribute( "value" );
            if( !variableValueString.isNull() )
            {
                QStringList list            = variableValueString.split( ' ' );
                QStringList::iterator it    = list.begin();
                QStringList::iterator itEnd = list.end();
                int i                       = 0;
                while( it != itEnd && i < nbElements )
                {
                    value[i] = ( *it ).toDouble();
                    ++it;
                    ++i;
                }
                return true;
            }
        }
        return false;
    }
};

//========================================================================
// Helper functions
//========================================================================

// Description:
// This is the main serialization function that will be called for every basic type
// supported by the serializers. It is there to provide a common syntax to serialize
// any type of data (even basic types).
template <class T>
bool Serialize( Serializer * serial, const char * attrName, T & value )
{
    return serial->Serialize( attrName, value );
}

template <class T>
bool Serialize( Serializer * serial, const char * attrName, T & value, int nbElements )
{
    return serial->Serialize( attrName, value, nbElements );
}

// Description:
// Specialization function to serialize std::vector containers
template <class T>
bool Serialize( Serializer * serial, const char * attrName, std::vector<T> & value, bool resize = true )
{
    if( serial->BeginSection( attrName ) )
    {
        int numberOfElements = (int)(value.size());
        Serialize( serial, "NumberOfElements", numberOfElements );
        if( serial->IsReader() && resize )
        {
            value.resize( numberOfElements );
        }

        typename std::vector<T>::iterator it = value.begin();
        int i                                = 0;
        while( it != value.end() )
        {
            QString elemName = QString( "Element_%1" ).arg( i );
            Serialize( serial, elemName.toUtf8().data(), *( it ) );
            ++i;
            ++it;
        }
        serial->EndSection();
        return true;
    }
    return false;
}

// Description:
// Specialization function to serialize std::vector of pointers containers
template <class T>
bool Serialize( Serializer * serial, const char * attrName, std::vector<T *> & value )
{
    if( serial->BeginSection( attrName ) )
    {
        int numberOfElements = value.size();
        Serialize( serial, "NumberOfElements", numberOfElements );
        if( serial->IsReader() )
        {
            value.resize( numberOfElements );
            for( int i = 0; i < numberOfElements; ++i ) value[i] = new T;
        }

        typename std::vector<T *>::iterator it = value.begin();
        int i                                  = 0;
        while( it != value.end() )
        {
            QString elemName = QString( "Element_%1" ).arg( i );
            Serialize( serial, elemName.toUtf8().data(), *( it ) );
            ++i;
            ++it;
        }
        serial->EndSection();
        return true;
    }
    return false;
}

// Description:
// Specialization function to serialize std::list containers
template <class T>
bool Serialize( Serializer * serial, const char * attrName, std::list<T> & value, bool resize = true )
{
    if( serial->BeginSection( attrName ) )
    {
        int numberOfElements = value.size();
        Serialize( serial, "NumberOfElements", numberOfElements );
        if( serial->IsReader() && resize )
        {
            value.resize( numberOfElements );
        }

        typename std::list<T>::iterator it = value.begin();
        int i                              = 0;
        while( it != value.end() )
        {
            QString elemName = QString( "Element_%1" ).arg( i );
            Serialize( serial, elemName.toUtf8().data(), *( it ) );
            ++i;
            ++it;
        }
        serial->EndSection();
        return true;
    }
    return false;
}

// Description:
// Specialization function to serialize QList of generic objects
template <class T>
bool Serialize( Serializer * serial, const char * attrName, QList<T> & value )
{
    if( serial->BeginSection( attrName ) )
    {
        int numberOfElements = value.size();
        Serialize( serial, "NumberOfElements", numberOfElements );
        if( serial->IsReader() )
        {
            value.reserve( numberOfElements );
            for( int i = 0; i < numberOfElements; ++i )
            {
                T t;
                value.push_back( t );
            }
        }

        typename QList<T>::iterator it = value.begin();
        int i                          = 0;
        while( it != value.end() )
        {
            QString elemName = QString( "Element_%1" ).arg( i );
            Serialize( serial, elemName.toUtf8().data(), *( it ) );
            ++i;
            ++it;
        }
        serial->EndSection();
        return true;
    }
    return false;
}

// Description:
// Specialization function to serialize QList of pointers containers
template <class T>
bool Serialize( Serializer * serial, const char * attrName, QList<T *> & value )
{
    if( serial->BeginSection( attrName ) )
    {
        int numberOfElements = value.size();
        Serialize( serial, "NumberOfElements", numberOfElements );
        if( serial->IsReader() )
        {
            value.reserve( numberOfElements );
            for( int i = 0; i < numberOfElements; ++i )
            {
                T * t = new T;
                value.push_back( t );
            }
        }

        typename QList<T *>::iterator it = value.begin();
        int i                            = 0;
        while( it != value.end() )
        {
            QString elemName = QString( "Element_%1" ).arg( i );
            Serialize( serial, elemName.toUtf8().data(), *( it ) );
            ++i;
            ++it;
        }
        serial->EndSection();
        return true;
    }
    return false;
}

template <class T, class R>
bool Serialize( Serializer * serial, const char * attrName, QPair<T, R> & value )
{
    if( serial->BeginSection( attrName ) )
    {
        Serialize( serial, "First", value.first );
        Serialize( serial, "Second", value.second );
        serial->EndSection();
        return true;
    }
    return false;
}

template <class T, class R>
bool Serialize( Serializer * serial, const char * attrName, std::pair<T, R> & value )
{
    if( serial->BeginSection( attrName ) )
    {
        Serialize( serial, "First", value.first );
        Serialize( serial, "Second", value.second );
        serial->EndSection();
        return true;
    }
    return false;
}

template <class K, class V>
bool Serialize( Serializer * serial, const char * attrName, std::map<K, V> & value )
{
    if( serial->BeginSection( attrName ) )
    {
        int numberOfElements = value.size();
        Serialize( serial, "NumberOfElements", numberOfElements );

        typename std::map<K, V>::iterator it = value.begin();
        for( int i = 0; i < numberOfElements; ++i )
        {
            QString elemName = QString( "Element_%1" ).arg( i );
            std::pair<K, V> nextValue;
            if( !serial->IsReader() )
            {
                nextValue = *it;
                ++it;
            }
            Serialize( serial, elemName.toUtf8().data(), nextValue );
            if( serial->IsReader() ) value[nextValue.first] = nextValue.second;
        }

        serial->EndSection();
        return true;
    }
    return false;
}

// Description:
// This macro should be used in header files of classes you want
// to be able to serialize. These classes have to implement
// a function with this signature:
//
// void Serialize( Serializer * serial )
//
#define ObjectSerializationMacro( className )                                       \
    bool Serialize( Serializer * serial, const char * attrName, className * value ) \
    {                                                                               \
        if( serial->BeginSection( attrName ) )                                      \
        {                                                                           \
            value->Serialize( serial );                                             \
            serial->EndSection();                                                   \
            return true;                                                            \
        }                                                                           \
        return false;                                                               \
    }                                                                               \
    bool Serialize( Serializer * serial, const char * attrName, className & value ) \
    {                                                                               \
        if( serial->BeginSection( attrName ) )                                      \
        {                                                                           \
            value.Serialize( serial );                                              \
            serial->EndSection();                                                   \
            return true;                                                            \
        }                                                                           \
        return false;                                                               \
    }

#define ObjectSerializationHeaderMacro( className )                                  \
    bool Serialize( Serializer * serial, const char * attrName, className * value ); \
    bool Serialize( Serializer * serial, const char * attrName, className & value );

#endif
