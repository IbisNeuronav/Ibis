/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#ifndef SCENEINFO_H
#define SCENEINFO_H

#include "serializer.h"
#include <qobject.h>
#include <string>
#include <vtkObject.h>
#include "vtkMatrix4x4.h"

#define ACQ_COLOR_RGB "RGB"
#define ACQ_COLOR_GRAYSCALE "Grayscale"
#define ACQ_COLOR_DEFAULT ACQ_COLOR_RGB

class SceneInfo : public QObject, public vtkObject
{
    Q_OBJECT

public:
    static SceneInfo * New() { return new SceneInfo; }

    vtkTypeMacro(SceneInfo,vtkObject);
    SceneInfo();
    virtual ~SceneInfo();

#define ACQ_TYPE_B_MODE "B-mode"
#define ACQ_TYPE_DOPPLER "Doppler"
#define ACQ_TYPE_POWER_DOPPLER "Power Doppler"
#define ACQ_TYPE_DEFAULT ACQ_TYPE_B_MODE

    void SetPatientIdentification(const QString id) {patientIdentification = id;}
    const QString GetPatientIdentification() {return patientIdentification;}
    void SetPatientName(const QString name) {patientName = name;}
    const QString GetPatientName() {return patientName;}
    void SetComment(const QString c) {comment = c;}
    const QString GetComment() {return comment;}
    void SetStudentName(const QString name) {studentName = name;}
    const QString GetStudentName() {return studentName;}
    void SetSurgeonName(const QString name) {surgeonName = name;}
    const QString GetSurgeonName() {return surgeonName;}
    void SetUltrasoundModel(const QString model) {ultrasoundModel = model;}
    const QString GetUltrasoundModel() {return ultrasoundModel;}
    void SetTrackerModel(const QString model) {trackerModel = model;}
    const QString GetTrackerModel() {return trackerModel;}
    void SetTrackerVersion(const QString version) {trackerVersion = version;}
    const QString GetTrackerVersion() {return trackerVersion;}
    void SetPointerModel(const QString model) {pointerModel = model;}
    const QString GetPointerModel() {return pointerModel;}
    void SetPointerCalibrationFile(const QString filename) {pointerCalibrationFile = filename;}
    const QString GetPointerCalibrationFile() {return pointerCalibrationFile;}
    void SetPointerRomFile(const QString filename) {pointerRomFile = filename;}
    const QString GetPointerRomFile() {return pointerRomFile;}
    void SetReferenceModel(const QString model) {referenceModel = model;}
    const QString GetReferenceModel() {return referenceModel;}
    void SetReferenceRomFile(const QString filename) {referenceRomFile = filename;}
    const QString GetReferenceRomFile() {return referenceRomFile;}
    void SetProbeModel(const QString model) {probeModel = model;}
    const QString GetProbeModel() {return probeModel;}
    void SetProbeCalibrationFile(const QString filename) {probeCalibrationFile = filename;}
    const QString GetProbeCalibrationFile() {return probeCalibrationFile;}
    void SetProbeRomFile(const QString filename) {probeRomFile = filename;}
    const QString GetProbeRomFile() {return probeRomFile;}
    void SetUsDepth(const QString depth) {usDepth = depth;}
    const QString GetUsDepth() {return usDepth;}
    void SetUsScale(const QString scale) {usScale = scale;}
    const QString GetUsScale() {return usScale;}
    void SetFrameRate(const QString rate) {frameRate = rate;}
    const QString GetFrameRate() {return frameRate;}
    void SetApplication(const QString app) {application = app;}
    const QString GetApplication() {return application;}
    void SetApplicationVersion(const QString version) {applicationVersion = version;}
    const QString GetApplicationVersion() {return applicationVersion;}
    void SetCalibrationDate(const QString date) {calibrationDate = date;}
    const QString GetCalibrationDate() {return calibrationDate;}
    void SetCalibrationMethod(const QString method) {calibrationMethod = method;}
    const QString GetCalibrationMethod() {return calibrationMethod;}
    void SetCalibrationVersion(const QString version) {calibrationVersion = version;}
    const QString GetCalibrationVersion() {return calibrationVersion;}
    void SetAcquisitionType(const QString type) {acquisitionType = type;}
    const QString GetAcquisitionType() {return acquisitionType;}
    void SetAcquisitionColor(const QString color) {acquisitionColor = color;}
    const QString GetAcquisitionColor() {return acquisitionColor;}
    
    void  SetSceneDirectory(const QString dir) {m_sessionDirectory = dir;}
    const QString GetSceneDirectory() {return m_sessionDirectory;}
    void  SetDirectorySet(bool set) {m_directorySet = set;}
    bool  GetDirectorySet() {return m_directorySet;}
    void  SetSessionFile(const QString file) {m_sessionFile = file;}
    const QString GetSessionFile() {return m_sessionFile;}

    virtual void Serialize( Serializer * serializer );
    void ShallowCopy(SceneInfo *source);
    void PrepareForMINCOutput( );

    vtkSetObjectMacro(UsProbeUnscaledCalibrationMatrix, vtkMatrix4x4);

//    STUDY_ATTRIBUTES *GetStudyAttributes() {return &studyAttributes;};
//    ACQUISITION_ATTRIBUTES *GetAcquisitionAttributes() {return &acquisitionAttributes;}
//    PATIENT_ATTRIBUTES *GetPatientAttributes() {return &patientAttributes;}

protected:
    QString patientIdentification;
    QString patientName;
    QString studentName;
    QString surgeonName;
    QString acquisitionDate;
    QString comment;
    QString ultrasoundModel;
    QString trackerModel;
    QString trackerVersion;
    QString pointerModel;
    QString pointerCalibrationFile;
    QString pointerRomFile;
    QString referenceModel;
    QString referenceRomFile;
    QString probeModel;
    QString probeCalibrationFile;
    QString probeRomFile;
    QString usDepth;
    QString usScale;
    QString frameRate;
    QString application;
    QString applicationVersion;
    QString calibrationDate;
    QString calibrationMethod;
    QString calibrationVersion;
    QString acquisitionType;
    QString acquisitionColor;
    
    QString m_sessionDirectory;
    QString m_sessionFile;
    bool m_directorySet;

    vtkMatrix4x4 *UsProbeUnscaledCalibrationMatrix;
    vtkMatrix4x4 *ActivePointerCalibrationMatrix;

/*
This is redundant, but as for now 2009/09/03 the fastest way to transfer
data to our vtkMINCWriter, until I find out how to use vtkMINCImageWriter
the one from vtk5.4 is messing up image orientation, spacing, etc.
I should not mix Qt with vtk in the vtkMINCWriter, so I have to change
QStrings to char pointers.
*/
 
//    STUDY_ATTRIBUTES studyAttributes;
//    ACQUISITION_ATTRIBUTES acquisitionAttributes;
//    PATIENT_ATTRIBUTES patientAttributes;
};

ObjectSerializationHeaderMacro( SceneInfo );

#endif // SCENEINFO_H
