/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __PolyDataAbstract_h_
#define __PolyDataAbstract_h_

#include "sceneobject.h"
#include "serializer.h"
#include <map>
#include <QVector>
#include <QObject>
#include <vtkProperty.h>
#include <vtkSmartPointer.h>

class vtkPolyData;
class vtkTransform;
class vtkActor;
class vtkDataSetAlgorithm;
class vtkImageData;
class vtkProbeFilter;
class vtkScalarsToColors;
class ImageObject;
class vtkClipPolyData;
class vtkPassThrough;
class vtkCutter;
class vtkPlane;
class vtkPlanes;

enum RenderingMode{ Solid = 0, Wireframe = 1, Both = 2 };


class PolyDataAbstract : public SceneObject
{
    
Q_OBJECT

public:
        
    static PolyDataAbstract * New() { return new PolyDataAbstract; }
    vtkTypeMacro(PolyDataAbstract,SceneObject);
    
    PolyDataAbstract();
    virtual ~PolyDataAbstract();
    virtual void Serialize( Serializer * ser ) override;
    virtual void Export() override;
    virtual bool IsExportable()  override { return true; }

    //virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets) = 0;

    vtkPolyData *GetPolyData();
    void SetPolyData( vtkPolyData *data );
    
    // Implementation of parent virtual method
    virtual void Setup( View * view ) override;
    virtual void Release( View * view ) override;

    int GetRenderingMode() { return this->renderingMode; }
    void SetRenderingMode( int mode );
    void SetScalarsVisible( int use );
    vtkGetMacro( ScalarsVisible, int );
    int GetLutIndex() { return LutIndex; }
    void SetLutIndex( int index );
    int GetVertexColorMode() { return VertexColorMode; }
    void SetVertexColorMode( int mode );
    double GetOpacity() { return this->Property->GetOpacity(); }
    void SetOpacity( double opacity );
    void SetColor(double r, double g, double b );
    void SetColor(double color[3]) { SetColor( color[0], color[1], color[2] ); }
    double * GetColor();
    void SetLineWidth( double w );
    void UpdateSettingsWidget();
    bool GetCrossSectionVisible() {return CrossSectionVisible;}
    void SetCrossSectionVisible( bool );
    void SetClippingEnabled( bool e );
    bool IsClippingEnabled() { return m_clippingOn; }
    void SetClippingPlanesOrientation( int plane, bool positive );
    bool GetClippingPlanesOrientation( int plane );

    bool IsUsingScalarSource();
    void UseScalarSource( bool use );
    ImageObject * GetScalarSource() { return this->ScalarSource; }
    void SetScalarSource( ImageObject * im );

    void SavePolyData( QString &fileName );
     
public slots:

    void OnStartCursorInteraction();
    void OnEndCursorInteraction();
    void OnCursorPositionChanged();
    void OnReferenceChanged();
    void OnScalarSourceDeleted();
    void OnScalarSourceModified();

signals:

    void ObjectViewChanged();
    
protected:

    // SceneObject overrides
    virtual void Hide() override;
    virtual void Show() override;
    virtual void ObjectAddedToScene() override;
    virtual void ObjectAboutToBeRemovedFromScene() override;
    virtual void InternalPostSceneRead() override;

    void UpdateClippingPlanes();
    void UpdateCuttingPlane();
    void UpdatePipeline();
    vtkScalarsToColors * GetCurrentLut();
    void InitializeClippingPlanes();
        
    vtkPolyData *PolyData;
    vtkSmartPointer<vtkPassThrough> m_clippingSwitch;
    vtkSmartPointer<vtkPassThrough> m_colorSwitch;

    int LutIndex;
    vtkSmartPointer<vtkScalarsToColors> CurrentLut;
    ImageObject * ScalarSource;
    vtkSmartPointer<vtkScalarsToColors> LutBackup;
    vtkSmartPointer<vtkProbeFilter> ProbeFilter;
    vtkSmartPointer<vtkProperty> Property;
    vtkSmartPointer<vtkProperty> m_2dProperty;
    
    typedef std::map< View*,vtkSmartPointer<vtkActor> > PolyDataObjectViewAssociation;
    PolyDataObjectViewAssociation polydataObjectInstances;
    
    int       renderingMode;  // one of VTK_POINTS, VTK_WIREFRAME or VTK_SURFACE
    int       ScalarsVisible;  // Whether scalars in the PolyData are used to color the object or not.
    int       VertexColorMode;   // 0 : use scalars in data, 1 : get scalars from object ScalarSourceObjectId
    int       ScalarSourceObjectId;

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


ObjectSerializationHeaderMacro( PolyDataAbstract );

#endif