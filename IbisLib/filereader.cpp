/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "filereader.h"
#include <vtkMNIOBJReader.h>
#include <vtkTagReader.h>
#include <vtkPolyDataReader.h>
#include <vtkProperty.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkOBJReader.h>
#include <vtkPLYReader.h>
#include <vtkDataObjectReader.h>
#include <vtkStructuredPointsReader.h>
#include <vtkErrorCode.h>
#include <vtkImageData.h>
#include <vtkTransform.h>
#include <vtkSmartPointer.h>
#include <vtkEventQtSlotConnect.h>
#include "imageobject.h"
#include "polydataobject.h"
#include "pointsobject.h"
#include "ibisapi.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include <QProcess>
#include <QStringList>
#include <QMessageBox>

#include <itkImageIOFactory.h>

const QString FileReader::MINCToolsPathVarName = "MINCToolsDirectory";

FileReader::FileReader(QObject *parent)
    : QThread(parent)

{
    m_currentFileIndex = 0;
    m_progress = 0.0;
    m_selfAllocParams = false;
    m_params = nullptr;
    m_fileProgressEvent = vtkEventQtSlotConnect::New();
    m_ibisAPI = nullptr;
}

FileReader::~FileReader()
{
    m_fileProgressEvent->Delete();
    if( m_params && m_selfAllocParams )
        delete m_params;
}

static QStringList mincConvertPaths = QStringList()
        << "/usr/bin/mincconvert"
        << "/usr/local/bin/mincconvert"
        << "/usr/local/minc/bin/mincconvert"
        << "/usr/local/mni/minc/bin/mincconvert"
        << "/opt/minc/bin/mincconvert"
        << "/opt/minc-itk4/bin/mincconvert";

static QStringList mincCalcPaths = QStringList()
        << "/usr/bin/minccalc"
        << "/usr/local/bin/minccalc"
        << "/usr/local/minc/bin/minccalc"
        << "/usr/local/mni/minc/bin/minccalc"
        << "/opt/minc/bin/minccalc"
        << "/opt/minc-itk4/bin/minccalc";

QString FindExecutable( QStringList & candidatePaths )
{
    foreach( QString path, candidatePaths )
    {
        QFileInfo info( path );
        if( info.exists() && info.isExecutable() )
        {
            return path;
        }
    }
    return QString("");
}

bool FileReader::FindMincConverter()
{
    m_mincconvert = FindExecutable( mincConvertPaths );
    m_minccalc = FindExecutable( mincCalcPaths );
    return ( !m_mincconvert.isEmpty() && !m_minccalc.isEmpty() );
}

bool FileReader::HasMincConverter()
{
    return !m_mincconvert.isEmpty();
}

bool FileReader::FindMINCTool( QString candidate )
{
    QFileInfo info( candidate );
    if( info.exists() && info.isExecutable() )
    {
        return true;
    }
    return false;
}

void FileReader::SetIbisAPI( IbisAPI *api )
{
    m_ibisAPI = api;
    if( m_ibisAPI )
    {
        QString  mincDir = m_ibisAPI->GetCustomPath( MINCToolsPathVarName );
        if( mincDir != QString::null && !mincDir.isEmpty() )
        {
            if( mincDir.at( mincDir.count()-1 ) != '/' )
                mincDir.append( "/" );
            m_mincconvert.clear();
            m_mincconvert.append( mincDir );
            m_mincconvert.append( "mincconvert" );
            bool ok = FindMINCTool( m_mincconvert );
            m_minccalc.clear();
            m_minccalc.append( mincDir );
            m_minccalc.append( "minccalc" );
            ok = ok && FindMINCTool( m_minccalc );
            if( !ok )
            {
                m_ibisAPI->UnRegisterCustomPath( MINCToolsPathVarName );
                ok = FindMincConverter();
                if( ok )
                {
                    QFileInfo fi( m_mincconvert );
                    m_ibisAPI->RegisterCustomPath( MINCToolsPathVarName, fi.absolutePath() );
                }
                else
                {
                    m_ibisAPI->RegisterCustomPath( MINCToolsPathVarName, "" );
                    m_mincconvert.clear();
                    m_minccalc.clear();
                }
            }
        }
        else
        {
            bool ok = FindMincConverter();
            if( ok )
            {
                QFileInfo fi( m_mincconvert );
                m_ibisAPI->RegisterCustomPath( MINCToolsPathVarName, fi.absolutePath() );
            }
            else
            {
                m_ibisAPI->RegisterCustomPath( MINCToolsPathVarName, "" );
                m_mincconvert.clear();
                m_minccalc.clear();
            }
        }
    }
}

bool FileReader::IsMINC1( QString fileName )
{
    FILE *fp = fopen( fileName.toUtf8().data(), "rb" );
    if( fp )
    {
        char first4[4];
        size_t count = fread( first4, 4, 1, fp );
        fclose( fp );

        if( count == 1 &&
            first4[0] == 'C' &&
            first4[1] == 'D' &&
            first4[2] == 'F' &&
            first4[3] == '\001' )
        {
            return true;
        }
    }
    return false;
}

bool FileReader::ConvertMINC1toMINC2( QString &inputileName, QString &outputileName, bool isVideoFrame )
{
    if( m_mincconvert.isEmpty() )
    {
        QString tmp("File ");
        tmp.append( inputileName + " is of MINC1 type and needs to be coverted to MINC2.\n" +
                    "Tool mincconvert was not found in standard paths on your file system.\n" +
                    "Please convert using command: \nmincconvert -2 <input> <output>");
        QMessageBox::critical( 0, "Error", tmp, 1, 0 );
        return false;
    }
    QString dirname( QDir::homePath() );
    dirname.append( "/.ibis/tmp/" );
    QDir tmpDir( dirname );
    bool ok = true;
    if( !tmpDir.exists() )
        ok = tmpDir.mkdir( dirname );
    if( !ok )
    {
        QString tmp( "Cannot create directory: " );
        tmp.append( dirname );
        this->ReportWarning(tmp);
        return false;
    }
    outputileName.append( dirname );
    QFileInfo fi( inputileName );
    outputileName.append( fi.fileName() );
    QString program( m_mincconvert );
    QStringList arguments;
    ok = false;
    if( !isVideoFrame )
    {
        arguments << "-2" << inputileName << outputileName << "-clobber" ;
        QProcess *convertProcess = new QProcess(0);
        convertProcess->start(program, arguments);
        ok = convertProcess->waitForStarted();
        if( ok )
            ok = convertProcess->waitForFinished();
        delete convertProcess;
        return ok;
    }
    else
    {
        QFileInfo fi(outputileName);
        QString dirname( fi.absolutePath() );
        QDir tmpDir( dirname );
        QString tempFile( dirname );
        tempFile.append("/tempfile.mnc");
        arguments << "-2" << inputileName << tempFile << "-clobber" ;
        QProcess *convertProcess = new QProcess(0);
        convertProcess->start(program, arguments);
        ok = convertProcess->waitForStarted(5000);
        if( ok )
            ok = convertProcess->waitForFinished(5000);
        delete convertProcess;
        if( ok )
        {
            if( !m_minccalc.isEmpty() )
            {
                program = m_minccalc;
                arguments.clear();
                arguments <<  "-express" << "A[0]*255"  << tempFile <<  outputileName << "-clobber" ;
                QProcess *calcProcess = new QProcess(0);
                calcProcess->start(program, arguments);
                ok = calcProcess->waitForStarted();
                if( ok )
                    ok = calcProcess->waitForFinished();
                delete calcProcess;
                tmpDir.remove( tempFile );
            }
            else
            {
                QString tmp("File ");
                tmp.append( inputileName + " is an acquired frame of MINC1 type and needs to be converted to MINC2.\n" +
                            "Tool minccalc was not found in standard paths on your file system.\n" );
                QMessageBox::critical( 0, "Error", tmp, 1, 0 );
                return false;
            }
        }
        return ok;
    }
}

void FileReader::SetParams( OpenFileParams * params )
{
    m_params = params;
}

void FileReader::SetFileNames( QStringList & filenames )
{
    m_params = new OpenFileParams;
    m_selfAllocParams = true;
    for( int i = 0; i < filenames.size(); ++i )
    {
        OpenFileParams::SingleFileParam p;
        p.fileName = filenames[i];
        m_params->filesParams.push_back( p );
    }
}

void FileReader::GetReadObjects( QList<SceneObject*> & objects )
{
    for( int i = 0; i < m_params->filesParams.size(); ++i )
    {
        if( m_params->filesParams[i].loadedObject )
            objects.push_back( m_params->filesParams[i].loadedObject );
        if( m_params->filesParams[i].secondaryObject )
            objects.push_back( m_params->filesParams[i].secondaryObject );
    }
}

void FileReader::run()
{
    m_currentFileIndex = 0;
    m_progress = 0.0;
    m_warnings.clear();
    for( int i = 0; i < m_params->filesParams.size(); ++i )
    {
        OpenFileParams::SingleFileParam & param = m_params->filesParams[i];
        m_currentFileIndex = i;
        QList<SceneObject*> readObjects;
        OpenFile( readObjects, param.fileName, param.objectName, param.isLabel );
        if( readObjects.size() > 0 )
            param.loadedObject = readObjects[0];
        if( readObjects.size() > 1 )
            param.secondaryObject = readObjects[1];
    }

    // Push all read objects to main thread to be able to create connections without having to worry about type of connection
    QThread * mainThread = QApplication::instance()->thread();
    for( int i = 0; i < m_params->filesParams.size(); ++i )
    {
        OpenFileParams::SingleFileParam & param = m_params->filesParams[i];
        if( param.loadedObject )
            param.loadedObject->moveToThread( mainThread );
        if( param.secondaryObject )
            param.secondaryObject->moveToThread( mainThread );
    }

    m_progress = 1.0;
}

QString FileReader::GetCurrentlyReadFile()
{
    QString filename;
    if( m_currentFileIndex < m_params->filesParams.size() )
    {
        QFileInfo info( m_params->filesParams[ m_currentFileIndex ].fileName );
        filename = info.fileName();
    }
    return filename;
}

void FileReader::OnReaderProgress( vtkObject * caller, unsigned long /*event*/ )
{
    vtkAlgorithm * reader = vtkAlgorithm::SafeDownCast( caller );
    Q_ASSERT( reader );

    double fileProgress = reader->GetProgress();
    ReaderProgress( fileProgress );
}

void FileReader::ReaderProgress( double fileProgress )
{
    double ratio = double(1.0) / m_params->filesParams.size();
    m_progress = ratio * ( (double)m_currentFileIndex + fileProgress );
}

bool FileReader::OpenFile( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName,
                           bool isLabel )
{
    if( QFile::exists( filename ) )
    {
        QString fileToOpen( filename );
        QString fileMINC2;
        if( this->IsMINC1( filename ) )
        {
            if( this->m_mincconvert.isEmpty())
            {
                QString tmp("File ");
                tmp.append( filename + " is  of MINC1 type and needs to be converted to MINC2.\n" +
                            "Open Settings/Preferences and set path to the directory containing MINC tools.\n" );
                QMessageBox::critical( 0, "Error", tmp, 1, 0 );
                return false;
            }
            if( this->ConvertMINC1toMINC2( filename, fileMINC2, true ) )
                fileToOpen = fileMINC2;
            else
                return false;
        }
        // Try reading using ITK ( all itk supported formats )
        bool fileOpened = false;
        if( isLabel )
        {
            fileOpened = OpenItkLabelFile(readObjects, fileToOpen, dataObjectName );
        }
        else
        {
            fileOpened = OpenItkFile( readObjects, fileToOpen, dataObjectName );
        }

        // temporary files cannot be removed before the end of the session - remove on exit from the application or on load scene
        if( fileOpened )
            return true;

        // try tag file
        vtkTagReader * readerTag = vtkTagReader::New();
        if( readerTag->CanReadFile( filename.toUtf8().data() ) )
        {
            readerTag->Delete();
            if( OpenTagFile( readObjects, filename, dataObjectName ) )
                return true;
        }
        else
            readerTag->Delete();

        // try reading MNI obj format
        vtkMNIOBJReader * readerObj = vtkMNIOBJReader::New();
        if( readerObj->CanReadFile( filename.toUtf8().data() ) )
        {
            readerObj->Delete();
            if( OpenObjFile( readObjects, filename, dataObjectName ) )
                return true;
        }
        else
            readerObj->Delete();

        // Try reading wavefront obj format
        // if extension is .obj but we couldn't read MNI obj, then it must be Wavefront obj
        if( filename.endsWith( QString(".obj") ) )
        {
            if( OpenWavObjFile( readObjects, filename, dataObjectName ) )
                return true;
        }

        // Try ply format
        if( filename.endsWith( QString(".ply") ) )
        {
            if( OpenPlyFile( readObjects, filename, dataObjectName ) )
                return true;
        }

        // try vtp
        if( OpenVTPFile( readObjects, filename, dataObjectName ) )
            return true;

        // finally try vtk
        if( OpenVTKFile( readObjects, filename, dataObjectName ) )
            return true;
    }
    QString tmp("File \"");
    tmp.append(filename);
    tmp.append("\" could not be open.");
    this->ReportWarning(tmp);
    return false;
}


#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkVTKImageExport.h"
#include "vtkImageImport.h"
#include "vtkImageData.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "stringtools.h"
#include "ibisitkvtkconverter.h"
#include <string>
#include <typeinfo>
#include <QString>
#include <iostream>
#include <itkImageIOFactory.h>
#include <itkImageIOBase.h>

typedef itk::ImageIOBase   IOBase;
typedef itk::SmartPointer< IOBase > IOBasePointer;

void FileReader::PrintMetadata(itk::MetaDataDictionary &dict)
{
    //let's write some meta information if there is any
    for(itk::MetaDataDictionary::ConstIterator it=dict.Begin();it!=dict.End();++it)
    {
        itk::MetaDataObjectBase *bs=(*it).second;
        itk::MetaDataObject<std::string> * str=dynamic_cast<itk::MetaDataObject<std::string> *>(bs);
        if( strstr( (*it).first.c_str(), "dicom" ) == 0 )
        {
            if(str)
            {
                std::cout<<(*it).first.c_str()<<" = "<< str->GetMetaDataObjectValue().c_str()<<std::endl;
            }
            else
            {
                std::cout<<(*it).first.c_str()<<" type: "<< typeid(*bs).name()<<std::endl;
            }
        }
    }
}

bool FileReader::OpenItkFile( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName )
{
    // try to read
    typedef itk::ImageFileReader< IbisItkFloat3ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(filename.toUtf8().data());

    try
    {
        reader->Update();
    }
    catch( itk::ExceptionObject & err )
    {
        return false;
    }
    // Update progress. simtodo : do something smarter. Itk minc reader doesn't seem to support progress.
    ReaderProgress( .5 );

    IbisItkFloat3ImageType::Pointer itkImage = reader->GetOutput();
    ImageObject * image = ImageObject::New();
    SetObjectName( image, dataObjectName, filename );
    if( image->SetItkImage( itkImage ) )
    {
        readObjects.push_back( image );
        ReaderProgress( 1.0 );
        //    itk::MetaDataDictionary &dictionary = itkImage->GetMetaDataDictionary();
        //    this->PrintMetadata(dictionary);

        return true;
    }
    this->ReportWarning( "Ignoring incomplete data." );
    return false;
}

bool FileReader::OpenItkLabelFile( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName )
{
    // try to read
    typedef itk::ImageFileReader< IbisItkUnsignedChar3ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( filename.toUtf8().data() );

    try
    {
        reader->Update();
    }
    catch( itk::ExceptionObject & err )
    {
        return false;
    }
    // Update progress. simtodo : do something smarter. Itk minc reader doesn't seem to support progress.
    ReaderProgress( .5 );

    IbisItkUnsignedChar3ImageType::Pointer itkImage = reader->GetOutput();
    ImageObject * image = ImageObject::New();
    if( image->SetItkLabelImage( itkImage ) )
    {
        SetObjectName( image, dataObjectName, filename );
        readObjects.push_back( image );

        ReaderProgress( 1.0 );

        return true;
    }
    this->ReportWarning( "Ignoring incomplete data." );
    return false;
}

bool FileReader::OpenObjFile( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName )
{
    vtkMNIOBJReader * reader = vtkMNIOBJReader::New();
    reader->SetFileName( filename.toUtf8().data() );

    // Read file and monitor progress
    m_fileProgressEvent->Connect( reader, vtkCommand::ProgressEvent, this, SLOT(OnReaderProgress( vtkObject*, unsigned long) ), 0, 0.0, Qt::DirectConnection );
    reader->Update();
    m_fileProgressEvent->Disconnect( reader );

    PolyDataObject * object = PolyDataObject::New();
    object->SetPolyData( reader->GetOutput() );
    object->SetColor( reader->GetProperty()->GetColor() );
    reader->Delete();

    SetObjectName( object, dataObjectName, filename );
    readObjects.push_back( object );

    return true;
}

bool FileReader::OpenWavObjFile( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName )
{
    vtkOBJReader * reader = vtkOBJReader::New();
    reader->SetFileName( filename.toUtf8().data() );

    // Read file and monito progress
    m_fileProgressEvent->Connect( reader, vtkCommand::ProgressEvent, this, SLOT(OnReaderProgress( vtkObject*, unsigned long) ), 0, 0.0, Qt::DirectConnection );
    reader->Update();
    m_fileProgressEvent->Disconnect( reader );

    PolyDataObject * object = PolyDataObject::New();
    object->SetPolyData( reader->GetOutput() );
    reader->Delete();

    SetObjectName( object, dataObjectName, filename );
    readObjects.push_back( object );

    return true;
}

bool FileReader::OpenPlyFile( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName )
{
    vtkPLYReader * reader = vtkPLYReader::New();
    reader->SetFileName( filename.toUtf8().data() );

    // Read file and monitor progress
    m_fileProgressEvent->Connect( reader, vtkCommand::ProgressEvent, this, SLOT(OnReaderProgress( vtkObject*, unsigned long) ), 0, 0.0, Qt::DirectConnection );
    reader->Update();
    m_fileProgressEvent->Disconnect( reader );

    PolyDataObject * object = PolyDataObject::New();
    object->SetPolyData( reader->GetOutput() );
    reader->Delete();

    SetObjectName( object, dataObjectName, filename );
    readObjects.push_back( object );

    return true;
}

bool FileReader::OpenVTKFile( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName )
{
    vtkDataObjectReader * reader = vtkDataObjectReader::New();
    reader->SetFileName( filename.toUtf8().data() );
    reader->Update();
    m_fileProgressEvent->Disconnect( reader );

    bool res = false;
    if( reader->GetErrorCode() == vtkErrorCode::NoError )
    {
        if (reader->IsFilePolyData())
        {
            vtkPolyDataReader * polyReader = vtkPolyDataReader::New();
            polyReader->SetFileName( filename.toUtf8().data() );

            m_fileProgressEvent->Connect( polyReader, vtkCommand::ProgressEvent, this, SLOT(OnReaderProgress( vtkObject*, unsigned long) ), 0, 0.0, Qt::DirectConnection );
            polyReader->Update();
            m_fileProgressEvent->Disconnect( polyReader );

            PolyDataObject * object = PolyDataObject::New();
            object->SetPolyData( polyReader->GetOutput() );
            SetObjectName( object, dataObjectName, filename );
            readObjects.push_back( object );
            res = true;

            polyReader->Delete();
        }
        else if (reader->IsFileStructuredPoints())
        {
            vtkStructuredPointsReader *pReader = vtkStructuredPointsReader::New();
            pReader->SetFileName( filename.toUtf8().data() );

            m_fileProgressEvent->Connect( pReader, vtkCommand::ProgressEvent, this, SLOT(OnReaderProgress( vtkObject*, unsigned long) ), 0, 0.0, Qt::DirectConnection );
            pReader->Update();
            m_fileProgressEvent->Disconnect( pReader );

            ImageObject * image = ImageObject::New();
            image->SetImage( (vtkImageData *)pReader->GetOutput() );
            pReader->Delete();

            SetObjectName( image, dataObjectName, filename );
            readObjects.push_back( image );
            res = true;
        }
        else
        {
            ReportWarning( tr("Unsupported file format") );
        }
    }
    reader->Delete();
    return res;
}

bool FileReader::OpenVTPFile( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName )
{
    vtkXMLPolyDataReader * polyReader = vtkXMLPolyDataReader::New();
    bool res = false;
    if (polyReader->CanReadFile(filename.toUtf8().data()))
    {
        polyReader->SetFileName( filename.toUtf8().data() );

        m_fileProgressEvent->Connect( polyReader, vtkCommand::ProgressEvent, this, SLOT(OnReaderProgress( vtkObject*, unsigned long) ), 0, 0.0, Qt::DirectConnection );
        polyReader->Update();
        m_fileProgressEvent->Disconnect( polyReader );

        PolyDataObject * object = PolyDataObject::New();
        object->SetPolyData( polyReader->GetOutput() );
        SetObjectName( object, dataObjectName, filename );
        object->SetScalarsVisible(0);
        readObjects.push_back( object );
        res = true;

    }
    polyReader->Delete();
    return res;
}

bool FileReader::OpenTagFile( QList<SceneObject*> & readObjects, QString filename, const QString & dataObjectName )
{
    vtkTagReader * reader = vtkTagReader::New();
    reader->SetFileName( filename.toUtf8().data() );

    m_fileProgressEvent->Connect( reader, vtkCommand::ProgressEvent, this, SLOT(OnReaderProgress( vtkObject*, unsigned long) ), 0, 0.0, Qt::DirectConnection );
    reader->Update();
    m_fileProgressEvent->Disconnect( reader );

    vtkPoints *pts = reader->GetVolume( 0 );
    if (!pts)
    {
        reader->Delete();
        return false;
    }
    int i, n = pts->GetNumberOfPoints();
    QFileInfo info( filename );
    QString displayName;
    if (!(dataObjectName.isNull() || dataObjectName.isEmpty()))
        displayName = dataObjectName;
    else
    {
        if (reader->GetVolumeNames().size() > 0)
            displayName = QString(reader->GetVolumeNames()[0].c_str());
        if (displayName.isEmpty())
            displayName = info.baseName();
    }
    PointsObject * pointsObject = PointsObject::New();
    pointsObject->SetName(displayName);
    pointsObject->SetDataFileName( info.fileName() );
    pointsObject->SetFullFileName( info.absoluteFilePath() );
    for (i = 0; i < n; i++)
    {
        pointsObject->AddPoint( QString(reader->GetPointNames()[i].c_str()), pts->GetPoint(i) );
        pointsObject->SetPointTimeStamp( i, reader->GetTimeStamps()[i].c_str() );
    }
    pointsObject->SetSelectedPoint( 0 );
    readObjects.push_back( pointsObject );

    // is there a second set of points?
    // it will be added to the same parent
    if (reader->GetNumberOfVolumes() > 1)
    {
        displayName = QString(reader->GetVolumeNames()[1].c_str());
        if (displayName.isEmpty())
            displayName = info.baseName()+"*";
        PointsObject * pointsObject1 = PointsObject::New();
        pointsObject1->SetName(displayName);
        vtkPoints *pts1 = reader->GetVolume( 1 );
        for (i = 0; i < n; i++)
        {
            pointsObject1->AddPoint(QString(reader->GetPointNames()[i].c_str()), pts1->GetPoint(i));
            pointsObject1->SetPointTimeStamp( i, reader->GetTimeStamps()[i].c_str() );
        }
        pointsObject1->SetSelectedPoint( 0 );
        readObjects.push_back( pointsObject1 );
    }
    reader->Delete();
    return true;
}


bool FileReader::GetFrameDataFromMINCFile(QString filename, vtkImageData *img , vtkMatrix4x4 *mat )
{
    Q_ASSERT(img);
    Q_ASSERT(mat);
    QString fileToRead( filename );
    QString fileMINC2;
    if( this->IsMINC1( filename ) )
    {
        if( this->m_mincconvert.isEmpty())
        {
            QString tmp("File ");
            tmp.append( filename + " is an acquired frame of MINC1 type and needs to be converted to MINC2.\n" +
                        "Open Settings/Preferences and set path to the directory containing MINC tools.\n" );
            QMessageBox::critical( 0, "Error", tmp, 1, 0 );
            return false;
        }
        if( this->ConvertMINC1toMINC2( filename, fileMINC2, true ) )
            fileToRead = fileMINC2;
        else
            return false;
    }

    IOBasePointer io = itk::ImageIOFactory::CreateImageIO(fileToRead.toUtf8().data(), itk::ImageIOFactory::ReadMode );

    if(!io)
      throw itk::ExceptionObject("Unsupported image file type");

    io->SetFileName((fileToRead.toUtf8().data()));
    io->ReadImageInformation();

    size_t nc = io->GetNumberOfComponents();

    IbisItkVtkConverter *ItktovtkConverter = IbisItkVtkConverter::New();
    vtkSmartPointer<vtkTransform> tr = vtkSmartPointer<vtkTransform>::New();
    if( nc == 1 )
    {
        typedef itk::ImageFileReader< IbisItkUnsignedChar3ImageType > ReaderType;
        ReaderType::Pointer reader = ReaderType::New();
        reader->SetFileName( fileToRead.toUtf8().data() );

        try
        {
            reader->Update();
        }
        catch( itk::ExceptionObject & err )
        {
            return false;
        }

        IbisItkUnsignedChar3ImageType::Pointer itkImage = reader->GetOutput();
        vtkImageData * tmpImg = ItktovtkConverter->ConvertItkImageToVtkImage( itkImage, tr );
        img->DeepCopy( tmpImg);
        mat->DeepCopy( tr->GetMatrix( ) );
        ItktovtkConverter->Delete();
        return true;
    }

    // try to read
    typedef itk::ImageFileReader< IbisRGBImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( fileToRead.toUtf8().data() );

    try
    {
        reader->Update();
    }
    catch( itk::ExceptionObject & err )
    {
        return false;
    }

    IbisRGBImageType::Pointer itkImage = reader->GetOutput();
    vtkImageData * tmpImg = ItktovtkConverter->ConvertItkImageToVtkImage( itkImage, tr );
    img->DeepCopy( tmpImg);
    mat->DeepCopy( tr->GetMatrix( ) );
    ItktovtkConverter->Delete();
    return true;
}

bool FileReader::GetPointsDataFromTagFile( QString filename, PointsObject *pts1, PointsObject *pts2 )
{
    Q_ASSERT(pts1);
    Q_ASSERT(pts2);

    vtkTagReader * reader = vtkTagReader::New();
    reader->SetFileName( filename.toUtf8().data() );

    reader->Update();

    vtkPoints *pts = reader->GetVolume( 0 );
    if (!pts)
    {
        reader->Delete();
        return false;
    }
    int i, n = pts->GetNumberOfPoints();
    QFileInfo info( filename );
    QString displayName;
    if (reader->GetVolumeNames().size() > 0)
        displayName = QString(reader->GetVolumeNames()[0].c_str());
    if (displayName.isEmpty())
        displayName = info.baseName();
    pts1->SetName(displayName);
    pts1->SetDataFileName( info.fileName() );
    pts1->SetFullFileName( info.absoluteFilePath() );
    for (i = 0; i < n; i++)
    {
        pts1->AddPoint( QString(reader->GetPointNames()[i].c_str()), pts->GetPoint(i) );
        pts1->SetPointTimeStamp( i, reader->GetTimeStamps()[i].c_str() );
    }
    // is there a second set of points?
    // it will be added to the same parent
    if (reader->GetNumberOfVolumes() > 1)
    {
        displayName = QString(reader->GetVolumeNames()[1].c_str());
        if (displayName.isEmpty())
            displayName = info.baseName()+"*";
        pts2->SetName(displayName);
        vtkPoints *pts1 = reader->GetVolume( 1 );
        for (i = 0; i < n; i++)
        {
            pts2->AddPoint(QString(reader->GetPointNames()[i].c_str()), pts1->GetPoint(i));
            pts2->SetPointTimeStamp( i, reader->GetTimeStamps()[i].c_str() );
        }
    }
    reader->Delete();
    return true;
}

void FileReader::SetObjectName( SceneObject * obj, QString objName, QString filename )
{
    QFileInfo info( filename );
    obj->SetDataFileName( info.fileName() );
    obj->SetFullFileName( info.absoluteFilePath() );
    if (!objName.isEmpty())
        obj->SetName( objName );
    else
        obj->SetName( info.fileName() );
}

void FileReader::ReportWarning( QString warning )
{
    m_warnings.push_back( warning );
}
