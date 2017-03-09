/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __FileReader_h_
#define __FileReader_h_

#include <QThread>
#include <QStringList>
#include "itkMetaDataObject.h"
#include <itkMetaDataDictionary.h>

class SceneManager;
class SceneObject;
class vtkEventQtSlotConnect;
class vtkObject;
class ImageObject;
class PointsObject;

class OpenFileParams
{

public:

    struct SingleFileParam
    {
        SingleFileParam() : isReference(false), isLabel(false), isMINC1(false), loadedObject(0), secondaryObject(0), parent(0) {}
        QString fileName;
        QString objectName;
        bool isReference;
        bool isLabel;  // load image as label image instead of floats.
        bool isMINC1;
        SceneObject * loadedObject;
        SceneObject * secondaryObject;  // This is a hack to attach the second point object that can be found in PointsObjects
        SceneObject * parent;
    };
    OpenFileParams() : defaultParent(0) {}
    ~OpenFileParams() {}
    void AddInputFile( QString filename, QString objectName = QString() )
    {
        SingleFileParam p;
        p.fileName = filename;
        p.objectName = objectName;
        filesParams.push_back( p );
    }
    void SetAllFileNames( const QStringList & inFiles )
    {
        for( int i = 0; i < inFiles.size(); ++i )
        {
            AddInputFile( inFiles[i] );
        }
    }
    QList<SingleFileParam> filesParams;
    QString lastVisitedDir;
    SceneObject * defaultParent;
};

class FileReader : public QThread
{
    Q_OBJECT

public:

    FileReader( QObject * parent = 0 );
    ~FileReader();

    void SetParams( OpenFileParams * params ) { m_params = params; }
    void SetFileNames( QStringList & filenames );  // helper that eventually call SetParams
    void GetReadObjects( QList<SceneObject*> & objects );  // helper

    const QStringList & GetWarnings() { return m_warnings; }
    double GetProgress() { return m_progress; }
    QString GetCurrentlyReadFile();

    void PrintMetadata(itk::MetaDataDictionary &dict);
    bool FindMincConverter();
    bool IsMINC1( QString fileName );
    bool ConvertMINC1toMINC2(QString &inputileName, QString &outputileName , bool isVideoFrame = false );
    bool GetFrameDataFromMINCFile(QString filename, ImageObject *img);
    bool GetPointsDataFromTagFile( QString filename, PointsObject *pts1, PointsObject *pts2 );

private slots:

    void OnReaderProgress( vtkObject*, unsigned long );

protected:

    void ReaderProgress( double fileProgress );

    void run();

    bool OpenFile        (QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "", bool isLabel = false, bool isMINC1 = false );
    bool OpenItkFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenItkLabelFile( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenObjFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenWavObjFile  ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenPlyFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName );
    bool OpenVTKFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenVTPFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenTagFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );

    void SetObjectName( SceneObject * obj, QString objName, QString filename );
    void ReportWarning( QString warning );

    // Progress report
    int m_currentFileIndex;
    double m_progress;

    // define stuff to read
    bool m_selfAllocParams;
    OpenFileParams * m_params;

    // callbacks
    vtkEventQtSlotConnect * m_fileProgressEvent;

    // Error collecting
    QStringList m_warnings;

    // Path to mincconvert
    QString m_mincconvert;
    QString m_minccalc;
};

#endif
