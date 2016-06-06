/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef CAMERAOBJECT_H
#define CAMERAOBJECT_H

#include "sceneobject.h"
#include "hardwaremodule.h"
#include "view.h"
#include <map>
#include <QVector>
#include "SVL.h"

class vtkCamera;
class vtkActor;
class vtkSimpleProp3D;
class vtkIbisImagePlaneMapper;
class vtkAxesActor;
class vtkPolyData;
class vtkImageData;
class QProgressDialog;

class CameraIntrinsicParams
{
public:
    CameraIntrinsicParams();
    CameraIntrinsicParams & operator=( const CameraIntrinsicParams & other );
    void Serialize( Serializer * ser );
    double GetVerticalAngleDegrees() const;
    void SetVerticalAngleDegrees( double angle );
    double GetVerticalAngleRad() const;
    void SetVerticalAngleRad( double angle );

    double m_center[2];
    double m_focal[2];
    double m_distorsionK1;
    int m_imageSize[2];
    double m_reprojectionError;
};

ObjectSerializationHeaderMacro( CameraIntrinsicParams );


class CameraObject : public SceneObject, ViewController
{

    Q_OBJECT

public:

    static CameraObject * New() { return new CameraObject; }
    vtkTypeMacro( CameraObject, SceneObject );

    CameraObject();
    ~CameraObject();

    virtual void Serialize( Serializer * ser );
    virtual void Export();
    bool Import( QString & directory, QProgressDialog * progressDlg=0 );
    virtual bool IsExportable()  { return true; }

    // Replacing direct interface to tracked video source
    vtkImageData * GetVideoOutput();
    int GetImageWidth();
    int GetImageHeight();
    TrackerToolState GetState();
    vtkTransform * GetUncalibratedTransform();
    void SetCalibrationMatrix( vtkMatrix4x4 * mat );
    vtkMatrix4x4 * GetCalibrationMatrix();
    void AddClient();
    void RemoveClient();

    void SetMatrices( vtkMatrix4x4 * uncalibratedMatrix, vtkMatrix4x4 * calibrationMatrix );

    virtual bool Setup( View * view );
    virtual bool Release( View * view );
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets);

    void SetIntrinsicEditable( bool e ) { m_intrinsicEditable = e; }
    bool IsIntrinsicEditable() { return m_intrinsicEditable; }
    void SetExtrinsicEditable( bool e ) { m_extrinsicEditable = e; }
    bool IsExtrinsicEditable() { return m_extrinsicEditable; }
    void SetIntrinsicParams( const CameraIntrinsicParams & params );
    const CameraIntrinsicParams & GetIntrinsicParams() { return m_intrinsicParams; }
    double GetVerticalAngleDegrees() { return m_intrinsicParams.GetVerticalAngleDegrees(); }
    void SetVerticalAngleDegrees( double angle );
    void SetImageDistance( double distance );
    double GetImageDistance() { return m_imageDistance; }
    double GetGlobalOpacity() { return m_globalOpacity; }
    void SetGlobalOpacity( double opacity );
    void SetImageCenter( double x, double y );
    double * GetImageCenter() { return m_intrinsicParams.m_center; }
    double GetLensDistortion() { return m_intrinsicParams.m_distorsionK1; }
    void SetLensDistortion( double x );
    double GetLensDisplacement();
    void SetLensDisplacement( double d );
    bool IsTransparencyCenterTracked() { return m_trackedTransparencyCenter; }
    void SetTransparencyCenterTracked( bool t );
    void SetTransparencyCenter( double x, double y );
    bool IsUsingTransparency() { return m_useTransparency; }
    void SetUseTransparency( bool use );
    bool IsUsingGradient() { return m_useGradient; }
    void SetUseGradient( bool use );
    bool IsShowingMask() { return m_showMask; }
    void SetShowMask( bool show );
    double GetSaturation() { return m_saturation; }
    void SetSaturation( double s );
    double GetBrightness() { return m_brightness; }
    void SetBrightness( double b );
    double * GetTransparencyCenter() { return m_transparencyCenter; }
    void SetTransparencyRadius( double min, double max );
    double * GetTransparencyRadius() { return m_transparencyRadius; }
    bool IsCameraTrackable() { return m_trackable; }
    void SetCameraTrackable( bool t );
    void AlignSceneCameraWithThis();
    void SetTrackCamera( bool t );
    bool GetTrackCamera();
    bool IsTransformFrozen();
    void FreezeTransform();
    void UnFreezeTransform();
    void TakeSnapshot();
    void ToggleRecording();
    bool IsRecording();
    int GetNumberOfFrames() { return m_capturedFrames.size(); }
    void AddFrame( vtkImageData * image, vtkMatrix4x4 * uncalMat );
    void SetCurrentFrame( int frame );
    int GetCurrentFrame() { return m_currentFrame; }

    // ViewController implementation
    void ReleaseControl( View * v );

    // Transform point from world to image space
    void WorldToImage( double * worldPos, double & xIm, double & yIm );

    // Drawing overlay on the image
    void DrawLine( double x1, double y1, double x2, double y2, double color[4] );
    void DrawPath( std::vector< Vec2 > & points, double color[4] );
    void DrawWorldPath( std::vector< Vec3 > & points, double color[4] );
    void DrawRect( double x, double y, double width, double height, double color[4] );
    void ClearDrawing();

signals:

    void VideoUpdatedSignal();

protected slots:

    void VideoUpdatedSlot();

protected:

    void InternalDrawPath( std::vector< Vec3 > & p3d, double color[4] );
    virtual void Hide();
    virtual void Show();

    virtual void ObjectAddedToScene();
    virtual void ObjectAboutToBeRemovedFromScene();
    virtual void InternalWorldTransformChanged();
    void InternalSetIntrinsicParams();
    void UpdateGeometricRepresentation();
    void SetLocalUncalibratedTransform( vtkTransform * t );
    virtual void UpdateWorldTransform();
    void UpdateVtkCamera();
    vtkRenderer * GetCurrentRenderer( View * v );

    void CreateCameraRepresentation();
    QString FindNextSnapshotName();

    void WriteMatrix( vtkMatrix4x4 * mat, QString filename );
    void WriteMatrices( std::vector< vtkMatrix4x4 * > & matrices, QString dirName );
    void ReadMatrix( QString filename, vtkMatrix4x4 * mat );
    void ReadMatrices( std::vector< vtkMatrix4x4 * > & matrices, QString dirName );
    void WriteImages( QString dirName, QProgressDialog * progressDlg = 0 );
    void ReadImages( int nbImages, QString dirName, QProgressDialog * progressDlg = 0 );

    struct PerViewElements
    {
        vtkActor * cameraActor;
        vtkSimpleProp3D * cameraImageActor;
        vtkIbisImagePlaneMapper * cameraImageMapper;
        vtkAxesActor * cameraAxesActor;
        vtkAxesActor * cameraTrackerAxesActor;
        vtkCamera * cameraBackup;
        std::vector< vtkProp3D * > anotations;
    };

    typedef std::map<View*,PerViewElements> PerViewElementCont;
    PerViewElementCont m_perViewElements;

    void ClearDrawingOneView( View * v, PerViewElements & elem );

    vtkCamera * m_camera;
    vtkPolyData * m_cameraPolyData;
    vtkTransform * m_imageTransform;
    CameraIntrinsicParams m_intrinsicParams;
    bool m_intrinsicEditable;
    bool m_extrinsicEditable;
    double m_imageDistance;
    double m_globalOpacity;
    double m_saturation;
    double m_brightness;
    bool m_useTransparency;
    bool m_useGradient;
    bool m_showMask;
    bool m_trackedTransparencyCenter;
    double m_transparencyCenter[2];
    double m_transparencyRadius[2];
    vtkTransform * m_uncalibratedWorldTransform;

    // alternative to m_trackedVideoSource in case we want to show a static image and its transforms
    int m_currentFrame;
    QList<vtkImageData *> m_capturedFrames;
    std::vector< vtkMatrix4x4 * > m_uncalibratedMatrices;
    vtkTransform * m_uncalibratedTransform;
    vtkTransform * m_calibrationTransform;
    vtkTransform * m_lensDisplacementTransform;
    vtkTransform * m_opticalCenterTransform;

    // Hold data used for recording
    CameraObject * m_recordingCamera;

    bool m_trackable;
    bool m_trackingCamera;
    int m_cachedImageSize[2];  // used to determine when image size is changed

};

ObjectSerializationHeaderMacro( CameraObject );

#endif
