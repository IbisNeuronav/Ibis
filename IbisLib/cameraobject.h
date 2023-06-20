/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __CameraObject_h_
#define __CameraObject_h_

#include <vtkSmartPointer.h>

#include <QObject>
#include <QVector>
#include <map>

#include "SVL.h"
#include "hardwaremodule.h"
#include "trackedsceneobject.h"
#include "view.h"

class TrackedVideoBuffer;
class vtkCamera;
class vtkActor;
class vtkSimpleProp3D;
class vtkIbisImagePlaneMapper;
class vtkAxesActor;
class vtkPolyData;
class vtkImageData;
class vtkPassThrough;
class vtkAlgorithmOutput;
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

    // Internal representation is independent of the image resolution
    // center and focal length are expressed as a percentage of the image width and height
    double m_center[2];
    double m_focal[2];
    double m_distorsionK1;
    double m_reprojectionError;  // cached reprojection error of last calibration
};

ObjectSerializationHeaderMacro( CameraIntrinsicParams );

class CameraObject : public TrackedSceneObject, ViewController
{
    Q_OBJECT

public:
    static CameraObject * New() { return new CameraObject; }
    vtkTypeMacro( CameraObject, TrackedSceneObject );

    CameraObject();
    ~CameraObject() override;

    virtual void Serialize( Serializer * ser ) override;
    virtual void SerializeTracked( Serializer * ser ) override;
    virtual void Export() override;
    bool Import( QString & directory, QProgressDialog * progressDlg = nullptr );
    virtual bool IsExportable() override { return true; }

    // Replacing direct interface to tracked video source
    void SetVideoInputConnection( vtkAlgorithmOutput * port );
    void SetVideoInputData( vtkImageData * image );
    vtkImageData * GetVideoOutput();
    int GetImageWidth();
    int GetImageHeight();
    void AddClient();
    void RemoveClient();

    virtual void Setup( View * view ) override;
    virtual void Release( View * view ) override;
    virtual void CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets ) override;

    // Mouse interaction with AR view
    virtual bool OnLeftButtonPressed( View * v, int x, int y, unsigned modifiers ) override;
    virtual bool OnLeftButtonReleased( View *, int, int, unsigned ) override;
    virtual bool OnMouseMoved( View *, int, int, unsigned ) override;

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
    void SetFocalPix( double fx, double fy );
    void GetFocalPix( double & fx, double & fy );
    void SetImageCenterPix( double x, double y );
    void GetImageCenterPix( double & x, double & y );
    double GetLensDistortion() { return m_intrinsicParams.m_distorsionK1; }
    void SetLensDistortion( double x );
    double GetLensDisplacement();
    void SetLensDisplacement( double d );
    bool IsTransparencyCenterTracked() { return m_trackTargetTransparencyCenter; }
    void SetTransparencyCenterTracked( bool t );
    void SetTransparencyCenter( double x, double y );
    bool IsUsingTransparency() { return m_useTransparency; }
    void SetUseTransparency( bool use );
    bool IsUsingGradient() { return m_useGradientTargetTransparencyModulation; }
    void SetUseGradient( bool use );
    bool IsShowingMask() { return m_showTargetTransparencyMask; }
    void SetShowMask( bool show );
    double GetSaturation() { return m_saturation; }
    void SetSaturation( double s );
    double GetBrightness() { return m_brightness; }
    void SetBrightness( double b );
    double * GetTransparencyCenter() { return m_transparencyCenter; }
    void SetTransparencyRadius( double min, double max );
    double * GetTransparencyRadius() { return m_transparencyRadius; }
    void AlignSceneCameraWithThis();
    void SetTrackCamera( bool t );
    bool GetTrackCamera();
    bool IsTransformFrozen();
    void FreezeTransform();
    void UnFreezeTransform();
    void TakeSnapshot();
    void ToggleRecording();
    bool IsRecording();
    int GetNumberOfFrames();
    void AddFrame( vtkImageData * image, vtkMatrix4x4 * uncalMat );
    void SetCurrentFrame( int frame );
    int GetCurrentFrame();

    // ViewController implementation
    void ReleaseControl( View * v ) override;

    // Transform point from world and window to image space
    void WorldToImage( double * worldPos, double & xIm, double & yIm );
    void WindowToImage( int x, int y, View * v, double & xIm, double & yIm );

    // Drawing overlay on the image
    void DrawLine( double x1, double y1, double x2, double y2, double color[4] );
    void DrawPath( std::vector<Vec2> & points, double color[4] );
    void DrawWorldPath( std::vector<Vec3> & points, double color[4] );
    void DrawRect( double x, double y, double width, double height, double color[4] );
    void DrawTarget( double x, double y, double radius, double color[4] );
    void ClearDrawing();

signals:

    void ParamsModified();
    void VideoUpdatedSignal();

protected slots:

    void ParamsModifiedSlot();
    void VideoUpdatedSlot();

protected:
    void ListenForIbisClockTick( bool listen );
    void InternalDrawPath( std::vector<Vec3> & p3d, double color[4] );

    // Base classes overrides
    virtual void Hide() override;
    virtual void Show() override;
    virtual void ObjectAddedToScene() override;
    virtual void ObjectAboutToBeRemovedFromScene() override;
    virtual void InternalPostSceneRead() override;

    void InternalSetIntrinsicParams();
    void InternalSetTrackCamera();
    void UpdateGeometricRepresentation();
    void UpdateVtkCamera();
    vtkRenderer * GetCurrentRenderer( View * v );
    void SerializeLocalParams( Serializer * ser );

    void CreateCameraRepresentation();
    QString FindNextSnapshotName();

    struct PerViewElements
    {
        vtkActor * cameraActor;
        vtkSimpleProp3D * cameraImageActor;
        vtkIbisImagePlaneMapper * cameraImageMapper;
        vtkAxesActor * cameraAxesActor;
        vtkAxesActor * cameraTrackerAxesActor;
        vtkCamera * cameraBackup;
        std::vector<vtkProp3D *> anotations;
    };

    typedef std::map<View *, PerViewElements> PerViewElementCont;
    PerViewElementCont m_perViewElements;

    void ClearDrawingOneView( View * v, PerViewElements & elem );

    vtkSmartPointer<vtkCamera> m_camera;
    vtkSmartPointer<vtkPolyData> m_cameraPolyData;
    vtkSmartPointer<vtkTransform> m_imageTransform;
    CameraIntrinsicParams m_intrinsicParams;
    bool m_intrinsicEditable;
    bool m_extrinsicEditable;
    double m_imageDistance;
    double m_globalOpacity;
    double m_saturation;
    double m_brightness;
    bool m_useTransparency;
    bool m_useGradientTargetTransparencyModulation;
    bool m_showTargetTransparencyMask;
    bool m_trackTargetTransparencyCenter;
    double m_transparencyCenter[2];
    double m_transparencyRadius[2];
    bool m_mouseMovingTransparency;

    vtkSmartPointer<vtkPassThrough> m_videoInputSwitch;

    // alternative to m_trackedVideoSource in case we want to show a static image and its transforms
    TrackedVideoBuffer * m_videoBuffer;
    vtkSmartPointer<vtkTransform> m_lensDisplacementTransform;
    vtkSmartPointer<vtkTransform> m_opticalCenterTransform;

    // Hold data used for recording
    vtkSmartPointer<CameraObject> m_recordingCamera;

    bool m_trackingCamera;
    int m_cachedImageSize[2];  // used to determine when image size is changed
};

ObjectSerializationHeaderMacro( CameraObject );

#endif
