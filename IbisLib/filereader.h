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
#include <QObject>
#include <itkMetaDataObject.h>
#include <itkMetaDataDictionary.h>

class SceneManager;
class SceneObject;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkImageData;
class vtkMatrix4x4;
class ImageObject;
class PointsObject;
class IbisAPI;
class OpenFileParams;

class FileReader : public QThread
{
    Q_OBJECT

public:

    FileReader( QObject * parent = 0 );
    ~FileReader();

    static const QString MINCToolsPathVarName;

    void SetParams( OpenFileParams * params );
    void SetFileNames( QStringList & filenames );  // helper that eventually call SetParams
    void GetReadObjects( QList<SceneObject*> & objects );  // helper

    const QStringList & GetWarnings() { return m_warnings; }
    double GetProgress() { return m_progress; }
    QString GetCurrentlyReadFile();

    void PrintMetadata(itk::MetaDataDictionary &dict);
    bool FindMincConverter();
    bool HasMincConverter();
    bool IsMINC1( QString fileName );
    bool ConvertMINC1toMINC2(QString &inputileName, QString &outputileName , bool isVideoFrame = false );
    bool GetFrameDataFromMINCFile(QString filename, vtkImageData *img , vtkMatrix4x4 *mat );
    bool GetPointsDataFromTagFile( QString filename, PointsObject *pts1, PointsObject *pts2 );

    void SetIbisAPI(IbisAPI *api );

private slots:

    void OnReaderProgress( vtkObject*, unsigned long );

protected:

    void ReaderProgress( double fileProgress );

    void run();

    bool OpenFile        (QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "", bool isLabel = false);
    bool OpenItkFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenItkLabelFile( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenObjFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenWavObjFile  ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenPlyFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName );
    bool OpenVTKFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenFIBFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenVTPFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenTagFile     ( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName = "" );

    void SetObjectName( SceneObject * obj, QString objName, QString filename );
    void ReportWarning( QString warning );

    bool FindMINCTool( QString candidate );

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

    // IbisAPI to get preferences
    IbisAPI *m_ibisAPI;
};

#endif
