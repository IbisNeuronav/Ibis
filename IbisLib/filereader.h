/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef FILEREADER_H
#define FILEREADER_H

#include <itkMetaDataDictionary.h>
#include <itkMetaDataObject.h>

#include <QObject>
#include <QStringList>
#include <QThread>

#include "ibisitkvtkconverter.h"

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

/**
 * @class   FileReader
 * @brief   Loading data from files of different types
 *
 * FileReader provides access to data stored in files.
 *
 * Following file types are supported:
 *
 * Minc file: *.mnc *.mnc2 *.mnc.gz *.MNC *.MNC2 *.MNC.GZ\n
 * Nifti file *.nii\n
 * Object file *.obj\n
 * PLY file *.ply\n
 * Tag file *.tag\n
 * VTK file: *.vtk *.vtp\n
 * FIB file *.fib
 *
 * The data is used to create a corresponding SceneObject.
 * Minc and Nifti are represented as ImageObject.
 * Object, PLY, VTK and VTP are represented as PolyDataObject.
 * FIB is represented as a TractogramObject.
 *
 * @sa
 * IbisAPI OpenFileParams SceneManager PointsObject SceneObject ImageObject TractogramObject
 */
class FileReader : public QThread
{
    Q_OBJECT

public:
    FileReader( QObject * parent = 0 );
    ~FileReader();

    /** Path to the directory containing MINC tools */
    static const QString MINCToolsPathVarName;

    /** Set all the attributes of the file to open */
    void SetParams( OpenFileParams * params );
    /** Helper function that eventually calls SetParams. */
    void SetFileNames( QStringList & filenames );
    /** Return a list of objects read in from open files. */
    void GetReadObjects( QList<SceneObject *> & objects );

    /** Warnings are accumulated during reading and then returned in a list of strings. */
    const QStringList & GetWarnings() { return m_warnings; }
    /** Return reading progress as a fraction between 0 and 1. */
    double GetProgress() { return m_progress; }
    /** Return the name of currently processed file. */
    QString GetCurrentlyReadFile();

    /** Used for debugging */
    void PrintMetadata( itk::MetaDataDictionary & dict );

    /** @name MINC1 and MINC2
     * @brief MINC1 dtecting and converting to MINC2
     */
    ///@{
    /** Find 2 executables - mincconvert and minccalc. */
    bool FindMincConverter();
    /** Were both mincconvert and minccalc found? */
    bool HasMincConverter();
    /** Is the file of MINC1 type? */
    bool IsMINC1( QString fileName );
    /** Convert MINC1 file to MINC2 file using mincconvert, if it is a frame from US acquisition, additionaly use
     * mincalc. */
    bool ConvertMINC1toMINC2( QString & inputileName, QString & outputileName, bool isVideoFrame = false );
    ///@}

    /** Return one or two PointsObjects loaded from a tag file. */
    bool GetPointsDataFromTagFile( QString filename, PointsObject * pts1, PointsObject * pts2 );

    /** @name US acquisitions
     * @brief Getting US acquisition files
     */
    ///@{
    /** Find out if the acquisition is grayscale (1 component) or RGB */
    int GetNumberOfComponents( QString filename );
    /** Load gray scale frame */
    bool GetGrayFrame( QString filename, IbisItkUnsignedChar3ImageType::Pointer itkImage );
    /** Load RGB frame.*/
    bool GetRGBFrame( QString filename, IbisRGBImageType::Pointer itkImage );
    ///@}

    /** IbisAPI is used to communicate between plugins and the core of ibis application
     *   In FileReader it is used to get from preferences the path to the directory containing MINC tools
     */
    void SetIbisAPI( IbisAPI * api );

private slots:

    void OnReaderProgress( vtkObject *, unsigned long );

protected:
    void ReaderProgress( double fileProgress );

    void run();

    bool OpenFile( QList<SceneObject *> & readObjects, QString filename, const QString & dataObjectName = "",
                   bool isLabel = false );
    bool OpenItkFile( QList<SceneObject *> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenItkLabelFile( QList<SceneObject *> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenObjFile( QList<SceneObject *> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenWavObjFile( QList<SceneObject *> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenPlyFile( QList<SceneObject *> & readObjects, QString filename, const QString & dataObjectName );
    bool OpenVTKFile( QList<SceneObject *> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenFIBFile( QList<SceneObject *> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenVTPFile( QList<SceneObject *> & readObjects, QString filename, const QString & dataObjectName = "" );
    bool OpenTagFile( QList<SceneObject *> & readObjects, QString filename, const QString & dataObjectName = "" );

    /** Set the name to show on the objects tree together with the file name and full [ath to the file. */
    void SetObjectName( SceneObject * obj, QString objName, QString filename );
    /** Push the warning string on the list of warnings. */
    void ReportWarning( QString warning );

    /** Check if the "candidate" exists and is executable .*/
    bool FindMINCTool( QString candidate );

    ///@{
    /** Progress report */
    int m_currentFileIndex;
    double m_progress;
    ///@}

    ///@{
    /** define stuff to read */
    bool m_selfAllocParams;
    OpenFileParams * m_params;
    ///@}

    /** callbacks */
    vtkEventQtSlotConnect * m_fileProgressEvent;

    /** Error collecting */
    QStringList m_warnings;

    /** Path to mincconvert */
    QString m_mincconvert;
    /** Path to minccalc */
    QString m_minccalc;

    /** IbisAPI is used to get the path to the directory containing MINC tools */
    IbisAPI * m_ibisAPI;
};

#endif
