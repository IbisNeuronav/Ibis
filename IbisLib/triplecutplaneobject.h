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
#include <vtkSmartPointer.h>
#include <QColor>
#include <QObject>

class ImageObject;
class QWidget;
class vtkMultiImagePlaneWidget;
class vtkEventQtSlotConnect;

/**
 * @class   TripleCutPlaneObject
 * @brief   Three perpendicular planes showing cross sections of the object in scene views
 *
 * The planes are always acting tohether, their section point forms the cursor.
 * In 2D views the planes always face the camera, parallel projection is used.
 * In 3D view planes are rotated together, perspective projection is used.
 * The real processing happens in vtkMultiImagePlaneWidget.
 * TripleCutPlaneObject controls adding and removing images and setting parameters for rendering.
 *
 */

class TripleCutPlaneObject : public SceneObject
{

Q_OBJECT

public:

	static TripleCutPlaneObject * New() { return new TripleCutPlaneObject; }
    vtkTypeMacro(TripleCutPlaneObject,SceneObject);

	TripleCutPlaneObject();
	virtual ~TripleCutPlaneObject();

    /** @name SceneObject reimplementation
     *  @brief Saving/loading and setting up the objects in the scene
     */
    ///@{
    /** Load/save TripleCutPlaneObject parameters */
    virtual void Serialize( Serializer * ser ) override;
    /** Do adjustments after loading all scene elements. */
    virtual void PostSceneRead() override;
    /** Setup objects in a view. */
    virtual void Setup( View * view ) override;
    /** Release objects in a view, called whenever all the objects in scene are removed. */
    virtual void Release( View * view ) override;
    /** ReleaseAllViews() will call Release() for every view. */
    virtual void ReleaseAllViews() override;
    ///@}

    /** @name  Settings widget
     *  @brief Create dialogs that will allow setting planes parameters
     */
    ///@{
    /** Main dialog. */
    virtual QWidget * CreateSettingsDialog( QWidget * parent ) override;
    /** Additional dialogs added as tabs. */
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets) override {}
    ///@}

    /** @name  Images
     *  @brief Manage images, reset planes.
     */
    ///@{
    /** Get the number of images. */
    int GetNumberOfImages() { return Images.size(); }
    /** Get an ImageObject knowing its index. */
    ImageObject * GetImage( int index );
    /** Add an ImageObject knowing its ID. */
    void AddImage( int imageID);
    /** Remove an ImageObject knowing its ID. */
    void RemoveImage(int imageID );
    /** Prepare to display. */
    void PreDisplaySetup() override;
    /** Set all three planes in the initial position, i.e. cutting in the middle. */
    void ResetPlanes();
    ///@}


    /** @name  Planes
     *  @brief Manage planes visibility.
     */
    ///@{
    /** Show/hide plane knowing its index. */
	void SetViewPlane( int planeIndex, int isOn );
    /** Check plane visibility. */
    int GetViewPlane( int planeIndex );
    /** Show/hide all three planes. */
    void SetViewAllPlanes( int isOn );
    ///@}

    // r
    /** @name  Cursor
     *  @brief Manage cursor visibility and color.
     */
    ///@{
    /** Is vcursor visible? */
    bool GetCursorVisible() { return CursorVisible; }
    /** Show/hide cursor. */
    void SetCursorVisibility( bool v );
    /** Set cursor color. */
    void SetCursorColor( const QColor & c );
    /** Get cursor color. */
    QColor GetCursorColor();
    ///@}

    /** @name  Interpolation
     *  @brief Manage data interpolation for reslice and display.
     */
    ///@{
    void SetResliceInterpolationType( int type );
    int GetResliceInterpolationType() { return m_resliceInterpolationType; }
    void SetDisplayInterpolationType( int type );
    int GetDisplayInterpolationType() { return m_displayInterpolationType; }
    ///@}

    /** @name  Slicing
     *  @brief Manage slice thickness and voxel combination type.
     */
    ///@{
    int GetSliceThickness() { return m_sliceThickness; }
    void SetSliceThickness( int );
    int GetSliceMixMode( int imageIndex ) { return m_sliceMixMode[ imageIndex ]; }
    void SetSliceMixMode( int imageIndex, int mode );
    ///@}

    /** @name  Blending
     *  @brief Manage color table and blending mode for individual images.
     */
    ///@{
    void SetColorTable( int imageIndex, int colorTableIndex );
    int GetColorTable( int imageIndex );
    void SetImageIntensityFactor( int imageIndex, double factor );
    double GetImageIntensityFactor( int imageIndex );
    int GetNumberOfBlendingModes();
    QString GetBlendingModeName( int blendingModeIndex );
    void SetBlendingModeIndex( int imageIndex, int blendingModeIndex );
    int GetBlendingModeIndex( int imageIndex ) { return m_blendingModeIndices[ imageIndex ]; }
    ///@}

    /** Get Planes position in local coordinates. */
    void GetPlanesPosition( double pos[3] );

    /** Determine whether the pos is in one of the 3 planes identified by planeType. Point is
     * in the plane if it is closer than .5 * voxel size of the reference volume.
     */
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
    virtual void MarkModified() override;
    void SetImageHidden(int imageID );

signals:
    void StartPlaneMoved(int);
    void PlaneMoved(int);
    void EndPlaneMove(int);

protected:

    virtual void ObjectAddedToScene() override;
    virtual void ObjectRemovedFromScene() override;
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

    vtkSmartPointer<vtkMultiImagePlaneWidget> Planes[3];
    int ViewPlanes[3];
    bool CursorVisible;
    int m_resliceInterpolationType;
    int m_displayInterpolationType;

	// callbacks
    vtkSmartPointer<vtkEventQtSlotConnect> PlaneInteractionSlotConnect;
    vtkSmartPointer<vtkEventQtSlotConnect> PlaneEndInteractionSlotConnect;

    //used only to set planes position in PostRead()
    double m_planePosition[3];

private:
    void UpdateAllPlanesVisibility();
    void UpdatePlanesVisibility(View *view );

};

ObjectSerializationHeaderMacro( TripleCutPlaneObject );

#endif // TRIPLECUTPLANEOBJECT_H
