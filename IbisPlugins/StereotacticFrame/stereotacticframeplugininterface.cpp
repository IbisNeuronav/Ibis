/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#include "stereotacticframeplugininterface.h"
#include "stereotacticframewidget.h"
#include <QtPlugin>
#include "vtkNShapeCalibrationWidget.h"
#include "vtkLandmarkTransform.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkAxesActor.h"
#include "vtkRenderer.h"
#include "application.h"
#include "scenemanager.h"
#include "view.h"
#include "polydataobject.h"
#include "imageobject.h"
#include "SVL.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include <float.h>

const Vec3 FrameLocators[4][4] = { { Vec3( 10.0, 140.0, -90.0 ),
                                     Vec3( 10.0, 10.0, -90.0 ),
                                     Vec3( 140.0, 140.0, -90.0 ),
                                     Vec3( 140.0, 10.0, -90.0 ) },
                                   { Vec3( 197.0, 140.0, -65.0 ),
                                     Vec3( 197.0, 10.0, -65.0 ),
                                     Vec3( 197.0, 140.0, 65.0 ),
                                     Vec3( 197.0, 10.0, 65.0 ) },
                                   { Vec3( 10.0, 140.0, 90.0 ),
                                     Vec3( 10.0, 10.0, 90.0 ),
                                     Vec3( 140.0, 140.0, 90.0 ),
                                     Vec3( 140.0, 10.0, 90.0 ) },
                                   { Vec3( -47.0, 140.0, -65.0 ),
                                     Vec3( -47.0, 10.0, -65.0 ),
                                     Vec3( -47.0, 140.0, 65.0 ),
                                     Vec3( -47.0, 10.0, 65.0 ) } };

StereotacticFramePluginInterface::StereotacticFramePluginInterface()
{
    m_manipulatorsOn = true;
    m_3DFrameOn = true;

    m_frameTransform = vtkTransform::New();
    m_adjustmentTransform = vtkTransform::New();
    m_landmarkTransform = vtkLandmarkTransform::New();
    m_landmarkTransform->SetModeToRigidBody();
    m_frameTransform->Concatenate( m_landmarkTransform );
    m_frameTransform->Concatenate( m_adjustmentTransform );

    m_frameRepresentation = 0;
    for( int i = 0; i < 4; i++ )
        m_manipulators[i] = 0;

    m_manipulatorsCallbacks = 0;
}

StereotacticFramePluginInterface::~StereotacticFramePluginInterface()
{
    m_frameTransform->Delete();
    m_landmarkTransform->Delete();
    m_adjustmentTransform->Delete();
    for( int i = 0; i < 4; i++ )
        if( m_manipulators[i] )
            m_manipulators[i]->Delete();
}

bool StereotacticFramePluginInterface::CanRun()
{
    return true;
}

QWidget * StereotacticFramePluginInterface::CreateTab()
{
    Initialize();

    StereotacticFrameWidget * widget = new StereotacticFrameWidget;
    widget->SetPluginInterface( this );

    connect( Application::GetSceneManager(), SIGNAL(ReferenceTransformChanged()), this, SLOT( OnReferenceTransformChanged() ));
    connect( Application::GetSceneManager(), SIGNAL(CursorPositionChanged()), this, SLOT(OnCursorMoved()) );

    return widget;
}

bool StereotacticFramePluginInterface::WidgetAboutToClose()
{
    if( m_manipulatorsOn )
        DisableManipulators();

    if( m_3DFrameOn )
        Disable3DFrame();

    disconnect( Application::GetSceneManager(), SIGNAL(ReferenceTransformChanged()), this, SLOT( OnReferenceTransformChanged() ));
    disconnect( Application::GetSceneManager(), SIGNAL(CursorPositionChanged()), this, SLOT(OnCursorMoved()) );

    return true;
}

void StereotacticFramePluginInterface::Initialize()
{
    ComputeInitialTransform();

    if( m_3DFrameOn )
        Enable3DFrame();

    if( m_manipulatorsOn )
        EnableManipulators();
}

void StereotacticFramePluginInterface::EnableManipulators()
{
    m_manipulatorsCallbacks = vtkEventQtSlotConnect::New();

    View * v = Application::GetSceneManager()->GetMainTransverseView( );
    Q_ASSERT_X( v, "StereotacticFramePluginInterface::CreateTab()", "No transverse view defined. It is needed by the StereotacticFrame plugin" );

    for( int i = 0; i < 4; ++i )
    {
        m_manipulators[i] = vtkNShapeCalibrationWidget::New();
        m_manipulators[i]->SetPoint1( 0.0, 0.0, 0.0 );   // default position, doesn't really matter
        m_manipulators[i]->SetPoint2( 200.0, 0.0, 0.0 );
        m_manipulators[i]->SetHandlesSize( 2.5 );
        m_manipulators[i]->SetInteractor( v->GetInteractor() );
        m_manipulators[i]->SetDefaultRenderer( v->GetOverlayRenderer() );
        m_manipulators[i]->SetPriority( 1.0 );
        m_manipulatorsCallbacks->Connect( m_manipulators[i], vtkCommand::InteractionEvent, this, SLOT(OnManipulatorsModified()) );
        m_manipulatorsCallbacks->Connect( m_manipulators[i], vtkCommand::StartInteractionEvent, this, SLOT(OnManipulatorsModified()) );
        m_manipulatorsCallbacks->Connect( m_manipulators[i], vtkCommand::EndInteractionEvent, this, SLOT(OnManipulatorsModified()) );
    }

    for( int i = 0; i < 4; ++i )
    {
        m_manipulators[i]->EnabledOn();
    }

    UpdateManipulators();

    // make sure the transverse view renders
    v->NotifyNeedRender();
}

void StereotacticFramePluginInterface::DisableManipulators()
{
    if( m_manipulatorsCallbacks )
    {
        m_manipulatorsCallbacks->Delete();
        m_manipulatorsCallbacks = 0;
    }

    View * v = Application::GetSceneManager()->GetMainTransverseView( );
    Q_ASSERT_X( v, "StereotacticFramePluginInterface::WidgetAboutToClose()", "No transverse view defined. It is needed by the StereotacticFrame plugin" );
    for( int i = 0; i < 4; i++ )
    {
        m_manipulators[i]->SetInteractor( 0 );
        m_manipulators[i]->EnabledOff();
        m_manipulators[i]->SetInteractor( 0 );
        m_manipulators[i]->Delete();
        m_manipulators[i] = 0;
    }
}

void StereotacticFramePluginInterface::SetShowManipulators( bool on )
{
    if( m_manipulatorsOn == on )
        return;

    m_manipulatorsOn = on;
    if( m_manipulatorsOn )
        EnableManipulators();
    else
        DisableManipulators();
}

void StereotacticFramePluginInterface::SetShow3DFrame( bool on )
{
    if( m_3DFrameOn == on )
        return;

    m_3DFrameOn = on;
    if( m_3DFrameOn )
        Enable3DFrame();
    else
        Disable3DFrame();
}

void StereotacticFramePluginInterface::GetCursorFramePosition( double pos[3] )
{
    // Get position of the cursor in reference image space
    double cursorPos[4];
    Application::GetSceneManager()->GetCursorPosition( cursorPos );
    cursorPos[3] = 1.0;

    // transform it into frame space
    double transformedPos[4];
    vtkMatrix4x4 * mat = vtkMatrix4x4::New();
    m_frameTransform->GetInverse( mat );
    mat->MultiplyPoint( cursorPos, transformedPos );
    mat->Delete();

    pos[0] = transformedPos[0];
    pos[1] = transformedPos[1];
    pos[2] = transformedPos[2];
}

void StereotacticFramePluginInterface::SetCursorFromFramePosition( double pos[3] )
{
    // convert framepos to homogenous 4x1 vector
    double framePos[4];
    for (int i=0; i<3; i++) {
        framePos[i] = pos[i];
    }
    framePos[3] = 1.0;

    // transform it into world space
    double transformedPos[4];
    vtkMatrix4x4 * mat = vtkMatrix4x4::New();
    m_frameTransform->GetMatrix( mat );
    mat->MultiplyPoint( framePos, transformedPos );
    mat->Delete();

    Application::GetSceneManager()->SetCursorPosition(transformedPos);
}

void StereotacticFramePluginInterface::OnCursorMoved()
{
    if( m_manipulatorsOn )
       UpdateManipulators();
    emit CursorMoved();
}

void StereotacticFramePluginInterface::OnManipulatorsModified()
{
    UpdateLandmarkTransform();
}

void StereotacticFramePluginInterface::OnReferenceTransformChanged()
{
    Initialize();
}

Mat4 vtkMatrix4x4ToMat4( vtkMatrix4x4 * mat )
{
    return Mat4( mat->Element[0][0], mat->Element[1][0], mat->Element[2][0], mat->Element[3][0],
                 mat->Element[0][1], mat->Element[1][1], mat->Element[2][1], mat->Element[3][1],
                 mat->Element[0][2], mat->Element[1][2], mat->Element[2][2], mat->Element[3][2],
                 mat->Element[0][3], mat->Element[1][3], mat->Element[2][3], mat->Element[3][3] );
}

void StereotacticFramePluginInterface::UpdateManipulators()
{
    // Get cursor position
    double cursorPos[3];
    Application::GetSceneManager()->GetCursorPosition( cursorPos );

    for( int nIndex = 0; nIndex < 4; ++nIndex )  // for each of the manipulators
    {
        // Plane definition in image coordinates
        Vec4 planePoint( 0.0, 0.0, cursorPos[2], 1.0 );
        Vec4 planeNormal( 0.0, 0.0, 1.0, 1.0 );

        // Get the corners of the N in image coordinates
        Mat4 worldToFrame = vtkMatrix4x4ToMat4( m_frameTransform->GetMatrix() );
        Vec4 markerPoints[3];
        for( int i = 0; i < 3; ++i  )
        {
            Vec4 n0( GetNPointPos( nIndex, i ), 1.0 );
            n0 *= worldToFrame;
            Vec4 n1( GetNPointPos( nIndex, i + 1 ), 1.0 );
            n1 *= worldToFrame;
            Vec4 diff = n1 - n0;
            double d = dot( planePoint - n0, planeNormal ) / dot( diff, planeNormal );
            markerPoints[i] = d * diff + n0;
        }

        m_manipulators[ nIndex ]->SetPoint1( markerPoints[0].Ref() );
        m_manipulators[ nIndex ]->SetPoint2( markerPoints[2].Ref() );
        m_manipulators[ nIndex ]->SetMiddlePoint( markerPoints[1].Ref() );
    }
}

void StereotacticFramePluginInterface::Enable3DFrame()
{
    ImageObject * ref = GetSceneManager()->GetReferenceDataObject();

    vtkPoints * pts = vtkPoints::New();

    for( int i = 0; i < 4; ++i )
        for( int j = 0; j < 4; ++j )
        {
            Vec3 p = GetNPointPos( i, j );
            pts->InsertNextPoint( p.Ref() );
        }

    vtkCellArray * lines = vtkCellArray::New();
    for( int i = 0; i < 4; ++i )  // each of the N shapes
        for( int j = 1; j < 4; ++j )  // each segment in the N shape
        {
            vtkIdType lineIndex[2];
            lineIndex[0] = i * 4 + j - 1;
            lineIndex[1] = i * 4 + j;
            lines->InsertNextCell( 2, lineIndex );
        }

    vtkPolyData * poly = vtkPolyData::New();
    poly->SetPoints( pts );
    pts->Delete();
    poly->SetLines( lines );
    lines->Delete();

    m_frameRepresentation = PolyDataObject::New();
    m_frameRepresentation->SetPolyData( poly );
    poly->Delete();
    m_frameRepresentation->SetListable( false );
    m_frameRepresentation->SetCanEditTransformManually( false );
    m_frameRepresentation->SetLocalTransform( m_frameTransform );

    GetSceneManager()->AddObject( m_frameRepresentation, ref );
}

void StereotacticFramePluginInterface::Disable3DFrame()
{
    GetSceneManager()->RemoveObject( m_frameRepresentation );
    m_frameRepresentation->Delete();
    m_frameRepresentation = 0;
}

const Vec3 & StereotacticFramePluginInterface::GetNPointPos( int nIndex, int pointInNIndex )
{
    Q_ASSERT_X( nIndex < 4 && pointInNIndex < 4, "StereotacticFramePluginInterface::GetNPointPos()", "Indexes out of range." );
    return FrameLocators[ nIndex ][ pointInNIndex ];
}


double alignmentMat[16] = { 0.0, 0.0, 1.0, 0.0,
                            -1.0, 0.0, 0.0, 0.0,
                            0.0, 1.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 1.0 };

void StereotacticFramePluginInterface::ComputeInitialTransform()
{
    // This is a transform to align spaces of image and frame that have very different conventions
    vtkTransform * alignmentTransform = vtkTransform::New();
    alignmentTransform->SetMatrix( alignmentMat );

    // Now, we compute a translation that centers bounding boxes of image and frame
    ImageObject * ref = GetSceneManager()->GetReferenceDataObject();
    if( !ref )
        return;
    double imageCenter[3] = { 0.0, 0.0, 0.0 };
    ref->GetCenter( imageCenter );
    Vec3 imCenter( imageCenter );

    // compute frame bound
    double frameBound[6] = { DBL_MAX, -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX, -DBL_MAX };
    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        {
            Vec4 point = Vec4( GetNPointPos( i, j ), 1.0 );
            double tPoint[4];
            alignmentTransform->MultiplyPoint( point.Ref(), tPoint );
            if( tPoint[0] < frameBound[0] )
                frameBound[0] = tPoint[0];
            if( tPoint[0] > frameBound[1] )
                frameBound[1] = tPoint[0];
            if( tPoint[1] < frameBound[2] )
                frameBound[2] = tPoint[1];
            if( tPoint[1] > frameBound[3] )
                frameBound[3] = tPoint[1];
            if( tPoint[2] < frameBound[4] )
                frameBound[4] = tPoint[2];
            if( tPoint[2] > frameBound[5] )
                frameBound[5] = tPoint[2];
        }
    }
    Vec3 frameCenter;
    frameCenter[0] = frameBound[0] + ( frameBound[1] - frameBound[0] ) / 2;
    frameCenter[1] = frameBound[2] + ( frameBound[3] - frameBound[2] ) / 2;
    frameCenter[2] = frameBound[4] + ( frameBound[5] - frameBound[4] ) / 2;

    Vec3 t = imCenter - frameCenter;

    vtkTransform * translation = vtkTransform::New();
    translation->Translate( t[0], t[1], t[2] );

    m_adjustmentTransform->Identity();
    m_adjustmentTransform->Concatenate( translation );
    m_adjustmentTransform->Concatenate( alignmentTransform );

    translation->Delete();
    alignmentTransform->Delete();
}

void StereotacticFramePluginInterface::UpdateLandmarkTransform()
{
    vtkPoints * frameSpacePoints = vtkPoints::New();
    vtkPoints * imageSpacePoints = vtkPoints::New();

    // for each n shape in the frame
    for( int nIndex = 0; nIndex < 4; ++nIndex )
    {
        // position in image space
        double imagePos[3];
        m_manipulators[ nIndex ]->GetMiddlePoint( imagePos );
        imageSpacePoints->InsertNextPoint( imagePos );

        // position in frame space
        double ratio = m_manipulators[ nIndex ]->GetMiddlePoint();
        Vec4 p2 = Vec4( GetNPointPos( nIndex, 2 ), 1.0 );
        Vec4 p1 = Vec4( GetNPointPos( nIndex, 1 ), 1.0 );
        Vec4 pFrame = p1 + ( p2 - p1 ) * ratio;

        double framePoint[4];
        m_adjustmentTransform->MultiplyPoint( pFrame.Ref(), framePoint );
        frameSpacePoints->InsertNextPoint( framePoint );
    }

    m_landmarkTransform->SetSourceLandmarks( frameSpacePoints );
    m_landmarkTransform->SetTargetLandmarks( imageSpacePoints );
    m_landmarkTransform->Modified();
    m_frameTransform->Modified(); // this should cause a redisplay of the 3D view since frame object will be modified
    frameSpacePoints->Delete();
    imageSpacePoints->Delete();
}
