/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TRIPLECUTPLANEOBJECT_H
#define TRIPLECUTPLANEOBJECT_H

#include <vector>
#include "ibistypes.h"
#include "serializer.h"
#include "sceneobject.h"
//#include "view.h"
#include <QColor>

class ImageObject;
class QWidget;
class vtkMultiImagePlaneWidget;
class vtkEventQtSlotConnect;

class TripleCutPlaneObject : public SceneObject
{

Q_OBJECT

public:

	static TripleCutPlaneObject * New() { return new TripleCutPlaneObject; }
    vtkTypeMacro(TripleCutPlaneObject,SceneObject);

	TripleCutPlaneObject();
	virtual ~TripleCutPlaneObject();

    // SceneObject implementation
    virtual void Serialize( Serializer * ser );
    virtual void PostSceneRead();
    virtual void Setup( View * view );
    virtual void Release( View * view );
    virtual void ReleaseAllViews();

    virtual QWidget * CreateSettingsDialog( QWidget * parent );
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets) {}

    // Original code
    int GetNumberOfImages() { return Images.size(); }
    ImageObject * GetImage( int index );
    void AddImage( int imageID);
    void RemoveImage(int imageID );
	void PreDisplaySetup();
	void ResetPlanes();
    vtkMultiImagePlaneWidget * GetPlane( int index ) { return (index < 3 && index >= 0) ? Planes[index] : 0; }

	// Manage plane visibility
	void SetViewPlane( int planeIndex, int isOn );
	int GetViewPlane( int planeIndex );
    void SetViewAllPlanes( int isOn );

    // Manage cursor visibility and color
    bool GetCursorVisible() { return CursorVisible; }
    void SetCursorVisibility( bool v );
    void SetCursorColor( const QColor & c );
    QColor GetCursorColor();

    // Manage data interpolation for reslice and display
    void SetResliceInterpolationType( int type );
    int GetResliceInterpolationType() { return m_resliceInterpolationType; }
    void SetDisplayInterpolationType( int type );
    int GetDisplayInterpolationType() { return m_displayInterpolationType; }

    // Manage slice thickness and voxel combination type
    int GetSliceThickness() { return m_sliceThickness; }
    void SetSliceThickness( int );
    int GetSliceMixMode( int imageIndex ) { return m_sliceMixMode[ imageIndex ]; }
    void SetSliceMixMode( int imageIndex, int mode );

    // Manage color table and blending mode for individual images
    void SetColorTable( int imageIndex, int colorTableIndex );
    int GetColorTable( int imageIndex );
    void SetImageIntensityFactor( int imageIndex, double factor );
    double GetImageIntensityFactor( int imageIndex );
    int GetNumberOfBlendingModes();
    QString GetBlendingModeName( int blendingModeIndex );
    void SetBlendingModeIndex( int imageIndex, int blendingModeIndex );
    int GetBlendingModeIndex( int imageIndex ) { return m_blendingModeIndices[ imageIndex ]; }

    void GetPlanesPosition( double pos[3] );

    // Description:
    // Determine whether the pos is in one of the 3 planes identified by planeType. Point is
    // in the plane if it is closer than .5 * voxel size of the reference volume.
    bool IsInPlane( VIEWTYPES planeType, double pos[3] );

public slots:

    void SetPlanesPosition( double * );
    void SetWorldPlanesPosition( double * pos );
    void PlaneStartInteractionEvent( vtkObject * caller, unsigned long event );
    void PlaneInteractionEvent( vtkObject * caller, unsigned long event );
    void PlaneEndInteractionEvent( vtkObject * caller, unsigned long event );
    // Adjust planes
    void AdjustAllImages(  );
    void UpdateLut( int imageID );
    void ObjectAddedSlot( int objectId );
    void ObjectRemovedSlot(int objectId );
    virtual void MarkModified();
    void SetImageHidden(int imageID );

signals:
    void StartPlaneMoved(int);
    void PlaneMoved(int);
    void EndPlaneMove(int);

protected:

    virtual void ObjectAddedToScene();
    virtual void ObjectRemovedFromScene();
    View * Get3DView();
	void Setup3DRepresentation( View * view );
	void Release3DRepresentation( View * view );
	void Setup2DRepresentation( int viewType, View * view );
	void Release2DRepresentation( int viewType, View * view );
	void CreatePlane( int viewType );

    int GetPlaneIndex( vtkObject * caller );
    void UpdateOtherPlanesPosition( int planeIndex );

    typedef std::vector< int > ImageContainer;
	ImageContainer Images;

    int m_sliceThickness;
    std::vector<int> m_sliceMixMode; // per-image
    std::vector<int> m_blendingModeIndices;

	vtkMultiImagePlaneWidget * Planes[3];
    int ViewPlanes[3];
    bool CursorVisible;
    int m_resliceInterpolationType;
    int m_displayInterpolationType;

	// callbacks
	vtkEventQtSlotConnect * PlaneInteractionSlotConnect;
    vtkEventQtSlotConnect * PlaneEndInteractionSlotConnect;

    //used only to set planes position in PostRead()
    double m_planePosition[3];

private:
    void UpdateAllPlanesVisibility();
    void UpdatePlanesVisibility(View *view );

};

ObjectSerializationHeaderMacro( TripleCutPlaneObject );

#endif // TRIPLECUTPLANEOBJECT_H
