/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __AbstractPolyDataObject_h_
#define __AbstractPolyDataObject_h_

#include <map>
#include <vtkProperty.h>
#include <vtkSmartPointer.h>

#include "sceneobject.h"
#include "serializer.h"

class vtkPolyData;
class vtkTransform;
class vtkActor;
class vtkClipPolyData;
class vtkPassThrough;
class vtkCutter;
class vtkPlane;
class vtkPlanes;

enum RenderingMode{ Solid = 0, Wireframe = 1, Both = 2 };


/**
 * @class   AbstractPolyDataObject
 * @brief   AbstractPolyDataObject is derived from SceneObject
 *
 * AbstractPolyDataObject is a superclass for classes that manipulate geometric structures consisting of vertices,
 * lines, polygons, and/or triangle strips.
 * While dealing with the geometry is left to vtkPolyData, AbstractPolyDataObject is taking care of setting parameters
 * such as color, line width, visibility, opacity via vtkProperty.
 * Clipping and cross sections with main planes is also defined in AbstractPolyDataObject.
 *
 *
 *  @sa SceneObject SceneManager PolyDataObject TractogramObject vtkPolyData vtkProperty
 */
class AbstractPolyDataObject : public SceneObject
{
    
Q_OBJECT

public:
    vtkTypeMacro(AbstractPolyDataObject,SceneObject);
    
    AbstractPolyDataObject();
    virtual ~AbstractPolyDataObject();
    virtual void Serialize( Serializer * ser ) override;
    
    // Implementation of parent virtual method
    virtual void Setup( View * view ) override;
    virtual void Release( View * view ) override;


    /** Get PolyData.*/
    vtkPolyData *GetPolyData();
    /** Set PolyData.*/
    void SetPolyData( vtkPolyData *data );

    /** Return current rendering mode, possible VTK_POINTS, VTK_WIREFRAME or VTK_SURFACE. */
    int GetRenderingMode() { return this->renderingMode; }
    /** Set rendering mode to VTK_POINTS, VTK_WIREFRAME or VTK_SURFACE.*/
    void SetRenderingMode( int mode );

    /** Control whether scalar data is used to color objects.*/
    void SetScalarsVisible( int use );
    /** Check if scalar data is used to color objects. */
    vtkGetMacro( ScalarsVisible, int );
    /** Get PolyData opacity. */
    double GetOpacity() { return this->Property->GetOpacity(); }
    /** Set PolyData opacity. */
    void SetOpacity( double opacity );
    /** Set PolyData color. */
    void SetColor(double r, double g, double b );
    /** Set PolyData color. */
    void SetColor(double color[3]) { SetColor( color[0], color[1], color[2] ); }
    /** Get PolyData color. */
    double * GetColor();
    /** Set line width. */
    void SetLineWidth( double w );
    /** Used to signal that PolyData is changed and settings need to be refreshed. */
    void UpdateSettingsWidget();
    /** Check if the cross section with main plains is visible.*/
    bool GetCrossSectionVisible() {return CrossSectionVisible;}
    /** Set visibility of the cross section with main plains is visible.*/
    void SetCrossSectionVisible( bool );
    /** Enable/disable clipping of the PolyData.*/
    void SetClippingEnabled( bool e );
    /** Check if the clipping of the PolyData is enabled.*/
    bool IsClippingEnabled() { return m_clippingOn; }
    /** Set the orientation of clipping planes. */
    void SetClippingPlanesOrientation( int plane, bool positive );
    /** Get the orientation of clipping planes. */
    bool GetClippingPlanesOrientation( int plane );
    /** Save PolyData in a file. */
    void SavePolyData( QString &fileName );
     
public slots:

    void OnStartCursorInteraction();
    void OnEndCursorInteraction();
    void OnCursorPositionChanged();
    void OnReferenceChanged();

signals:

    void ObjectViewChanged();
    
protected:

    virtual void UpdatePipeline() = 0;

    // SceneObject overrides
    virtual void Hide() override;
    virtual void Show() override;
    virtual void ObjectAddedToScene() override;
    virtual void ObjectAboutToBeRemovedFromScene() override;
    virtual void InternalPostSceneRead() override;

    void UpdateClippingPlanes();
    void UpdateCuttingPlane();
    void InitializeClippingPlanes();
        
    vtkPolyData *PolyData;
    vtkSmartPointer<vtkPassThrough> m_clippingSwitch;
    vtkSmartPointer<vtkPassThrough> m_colorSwitch;

    vtkSmartPointer<vtkProperty> Property;
    vtkSmartPointer<vtkProperty> m_2dProperty;
    
    typedef std::map< View*,vtkSmartPointer<vtkActor> > PolyDataObjectViewAssociation;
    PolyDataObjectViewAssociation polydataObjectInstances;
    
    int renderingMode;  // one of VTK_POINTS, VTK_WIREFRAME or VTK_SURFACE
    int ScalarsVisible;  // Whether scalars in the PolyData are used to color the object or not.

    // Cross section in 2d views
    vtkSmartPointer<vtkCutter> m_cutter[3];
    vtkSmartPointer<vtkPlane> m_cuttingPlane[3];
    bool CrossSectionVisible;

    // Clipping an octant from surface
    vtkSmartPointer<vtkClipPolyData> m_clipper;
    vtkSmartPointer<vtkTransform> m_referenceToPolyTransform;
    vtkSmartPointer<vtkPlanes> m_clippingPlanes;
    bool m_clippingOn;
    bool m_interacting;
};


ObjectSerializationHeaderMacro( AbstractPolyDataObject );

#endif
