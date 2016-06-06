/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "sceneinfo.h"

#include <stdio.h>
#include <qdatetime.h>
#include <qdir.h>
#include "ignsconfig.h"
#include "vtkXFMWriter.h"

ObjectSerializationMacro( SceneInfo );

SceneInfo::SceneInfo()
{
    QDateTime creationTime = QDateTime::currentDateTime();
    this->acquisitionDate = creationTime.toString(Qt::ISODate);
    this->studentName = "";
    this->ultrasoundModel = IGNS_ULTRASOUND_MODEL;
    this->trackerModel = IGNS_TRACKER_MODEL;
    this->application = "ibis";
    this->applicationVersion = QString("");
    this->ActivePointerCalibrationMatrix = 0;
    this->UsProbeUnscaledCalibrationMatrix = 0;

    m_directorySet = false;
    m_sessionDirectory = QDir::homePath() + "/" + IGNS_CONFIGURATION_SUBDIRECTORY;
}

SceneInfo::~SceneInfo()
{
}

#define IGNS_POINTER_CALIBRATION_FILE    "pointer_calibration.xfm"
#define IGNS_US_PROBE_CALIBRATION_FILE   "us_probe_calibration.xfm"
void SceneInfo::Serialize( Serializer * serializer )
{
    if(!serializer->IsReader())
    {
        QString calibrationFile( m_sessionDirectory );
        calibrationFile.append("/");
        vtkXFMWriter *writer = vtkXFMWriter::New();
        if (this->UsProbeUnscaledCalibrationMatrix)
        {
            calibrationFile.append(IGNS_US_PROBE_CALIBRATION_FILE);
            this->SetProbeCalibrationFile(calibrationFile);
            writer->SetFileName( calibrationFile.toUtf8().data() );
            writer->SetMatrix(this->UsProbeUnscaledCalibrationMatrix);
            writer->Write();
            calibrationFile.remove(IGNS_US_PROBE_CALIBRATION_FILE);
        }
        if (this->ActivePointerCalibrationMatrix)
        {
            calibrationFile.append(IGNS_POINTER_CALIBRATION_FILE);
            this->SetPointerCalibrationFile(calibrationFile);
            writer->SetFileName( calibrationFile.toUtf8().data() );
            writer->SetMatrix(this->ActivePointerCalibrationMatrix);
            writer->Write();
        }
        writer->Delete();
    }
    ::Serialize( serializer, "PatientIdentification", patientIdentification );
    ::Serialize( serializer, "PatientName", patientName );
    ::Serialize( serializer, "StudentName", studentName );
    ::Serialize( serializer, "SurgeonName", surgeonName );
    ::Serialize( serializer, "Comment", comment );
//    ::Serialize( serializer, "AcquisitionDate", acquisitionDate );
//    ::Serialize( serializer, "UltrasoundModel", ultrasoundModel );
//    ::Serialize( serializer, "TrackerModel", trackerModel );
//    ::Serialize( serializer, "TrackerVersion", trackerVersion );
//    ::Serialize( serializer, "PointerModel", pointerModel );
//    ::Serialize( serializer, "PointerCalibrationFile", pointerCalibrationFile );
//    ::Serialize( serializer, "PointerRomFile", pointerRomFile );
//    ::Serialize( serializer, "ReferenceModel", referenceModel );
//    ::Serialize( serializer, "ReferenceRomFile", referenceRomFile );
//    ::Serialize( serializer, "ProbeModel", probeModel );
//    ::Serialize( serializer, "ProbeCalibrationFile", probeCalibrationFile );
//    ::Serialize( serializer, "ProbeRomFile", probeRomFile );
//    ::Serialize( serializer, "usDepth", usDepth );
//    ::Serialize( serializer, "usScale", usScale );
//    ::Serialize( serializer, "application", application );
//    ::Serialize( serializer, "applicationVersion", applicationVersion );
//    ::Serialize( serializer, "calibrationDate", calibrationDate );
//    ::Serialize( serializer, "calibrationMethod", calibrationMethod );
//    ::Serialize( serializer, "calibrationVersion", calibrationVersion );
//    ::Serialize( serializer, "acquisitionType", acquisitionType );
//    ::Serialize( serializer, "acquisitionColor", acquisitionColor );
//    ::Serialize( serializer, "SessionDirectory", m_sessionDirectory );
}

void SceneInfo::ShallowCopy(SceneInfo *source)
{
    //copy only elements that are displayed in UI, others won't change during the session
    this->patientIdentification = source->GetPatientIdentification();
    this->patientName = source->GetPatientName();
    this->comment = source->GetComment();
    this->studentName = source->GetStudentName();
    this->surgeonName = source->GetSurgeonName();
    this->usDepth = source->GetUsDepth();
    this->usScale = source->GetUsScale();
    this->frameRate = source->GetFrameRate();
    this->acquisitionType = source->GetAcquisitionType();
    this->acquisitionColor = source->GetAcquisitionColor();
    m_sessionDirectory = source->GetSceneDirectory();
    m_sessionFile = source->GetSessionFile();
}

void SceneInfo::PrepareForMINCOutput( )
{
#if 0
    studyAttributes.clear();
    acquisitionAttributes.clear();
    patientAttributes.clear();
    // patient
    if (!patientName.isEmpty())
        patientAttributes[MIfull_name] = patientName.toStdString();
    if (!patientIdentification.isEmpty())
        patientAttributes[MIidentification] = patientIdentification.toStdString();
    // study
    if (!studentName.isEmpty())
        studyAttributes[MIoperator] = studentName.toStdString();
    if (!surgeonName.isEmpty())
        studyAttributes[MIattending_physician] = surgeonName.toStdString();
    studyAttributes[MImodality] = "Ultrasound";
    studyAttributes[MImanufacturer] = "ATL";
    if (!ultrasoundModel.isEmpty())
        studyAttributes[MIdevice_model] = ultrasoundModel.toStdString();
    if (!trackerModel.isEmpty())
        studyAttributes[MItracker_model] = trackerModel.toStdString();
    if (!trackerVersion.isEmpty())
        studyAttributes[MItracker_version] = trackerVersion.toStdString();
    if (!application.isEmpty())
        studyAttributes[MIapplication] = application.toStdString();
    if (!applicationVersion.isEmpty())
        studyAttributes[MIapplication_version] = applicationVersion.toStdString();
    if (!probeModel.isEmpty())
        studyAttributes[MItransducer_model] = probeModel.toStdString();
    if (!probeRomFile.isEmpty())
        studyAttributes[MItransducer_rom] = probeRomFile.toStdString();
    if (!probeCalibrationFile.isEmpty())
        studyAttributes[MItransducer_calibration] = probeCalibrationFile.toStdString();
    if (!pointerModel.isEmpty())
        studyAttributes[MIpointer_model] = pointerModel.toStdString();
    if (!pointerRomFile.isEmpty())
        studyAttributes[MIpointer_rom] = pointerRomFile.toStdString();
    if (!pointerCalibrationFile.isEmpty())
        studyAttributes[MIpointer_calibration] = pointerCalibrationFile.toStdString();
    if (!referenceModel.isEmpty())
        studyAttributes[MIreference_model] = referenceModel.toStdString();
    if (!referenceRomFile.isEmpty())
        studyAttributes[MIreference_rom] = referenceRomFile.toStdString();
    if (!calibrationMethod.isEmpty())
        studyAttributes[MIcalibration_method] = calibrationMethod.toStdString();
    if (!calibrationVersion.isEmpty())
        studyAttributes[MIcalibration_version] = calibrationVersion.toStdString();
    if (!calibrationDate.isEmpty())
        studyAttributes[MIcalibration_date] = calibrationDate.toStdString();
   // acquisition
    if (!usDepth.isEmpty())
        acquisitionAttributes[MIultrasound_depth] = usDepth.toStdString();
    if (!usScale.isEmpty())
        acquisitionAttributes[MIultrasound_scale] = usScale.toStdString();
    if (!acquisitionType.isEmpty())
        acquisitionAttributes[MIultrasound_acquisition_type] = acquisitionType.toStdString();
    if (!acquisitionColor.isEmpty())
        acquisitionAttributes[MIultrasound_acquisition_color] = acquisitionColor.toStdString();
    // comment and time stamp are passed directly to the MINC writer.
#endif
}

