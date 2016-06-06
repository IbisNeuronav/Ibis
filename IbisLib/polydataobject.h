/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __PolyDataObject_h_
#define __PolyDataObject_h_

#include "sceneobject.h"
#include "serializer.h"
#include <map>
#include <QVector>

class vtkPolyData;
class vtkTransform;
class vtkActor;
class vtkProperty;
class vtkTubeFilter;
class vtkDataSetAlgorithm;
class vtkImageData;
class vtkProbeFilter;
class vtkScalarsToColors;
class SurfaceCuttingPlane;
class PolyDataClipper;
class ImageObject;

enum RenderingMode{ Solid = 0, Wireframe = 1, Both = 2 };

class PolyDataObject : public SceneObject
{
    
Q_OBJECT

public:
        
    static PolyDataObject * New() { return new PolyDataObject; }
    vtkTypeMacro(PolyDataObject,SceneObject);
    
    PolyDataObject();
    virtual ~PolyDataObject();
    
    virtual void Serialize( Serializer * ser );
    virtual void Export();
    virtual bool IsExportable()  { return true; }

    vtkGetObjectMacro( PolyData, vtkPolyData );
    void SetPolyData( vtkPolyData * data );
    void SetProperty( vtkProperty * property );
    vtkGetObjectMacro( Property, vtkProperty );
    
    // Implementation of parent virtual method
	virtual bool Setup( View * view );
    virtual bool Release( View * view );

    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets);

    int GetRenderingMode() { return this->renderingMode; }
    void SetRenderingMode( int mode );
    void SetScalarsVisible( int use );
    vtkGetMacro( ScalarsVisible, int );
    int GetLutIndex() { return LutIndex; }
    void SetLutIndex( int index );
    int GetVertexColorMode() { return VertexColorMode; }
    void SetVertexColorMode( int mode );
    double GetOpacity() { return this->opacity; }
    void SetOpacity( double opacity );
    void SetColor(double color[3]);
    void SetLineWidth( double w );
    void UpdateSettingsWidget();
    void ShowCrossSection(bool);
    bool GetCrossSectionVisible() {return CrossSectionVisible;}
    void SetCrossSectionVisible( bool );
    void RemoveSurfaceFromOctant(int, bool);
    int GetSurfaceRemovedOctantNumber() {return SurfaceRemovedOctantNumber;}
    void SetTexture( vtkImageData * texImage );
    bool GetShowTexture() { return showTexture; }
    void SetShowTexture( bool show );
    void SetTextureFileName( QString );
    QString GetTextureFileName() { return textureFileName; }

    bool IsUsingScalarSource();
    void UseScalarSource( bool use );
    ImageObject * GetScalarSource() { return this->ScalarSource; }
    void SetScalarSource( ImageObject * im );
     
public slots:

    void UpdateOctant(int);
    void OnScalarSourceDeleted();
    void OnScalarSourceModified();

signals:

    void ObjectViewChanged();
    
protected:

    virtual void Hide();
    virtual void Show();
    void UpdateMapperPipeline();
    vtkScalarsToColors * GetCurrentLut();
    virtual void InternalPostSceneRead();
        
    vtkPolyData * PolyData;
    int LutIndex;
    vtkScalarsToColors * CurrentLut;
    vtkImageData * Texture;
    vtkDataSetAlgorithm * TextureMap;
    ImageObject * ScalarSource;
    vtkScalarsToColors * LutBackup;
    vtkProbeFilter * ProbeFilter;
    vtkProperty * Property;
    
    typedef std::map<View*,vtkActor*> PolyDataObjectViewAssociation;
    PolyDataObjectViewAssociation polydataObjectInstances;
    
    int       renderingMode;  // one of VTK_POINTS, VTK_WIREFRAME or VTK_SURFACE
    int       ScalarsVisible;  // Whether scalars in the PolyData are used to color the object or not.
    int       VertexColorMode;   // 0 : use scalars in data, 1 : get scalars from object ScalarSourceObjectId
    double    opacity;        // between 0 and 1
    double    objectColor[3];
    bool      showTexture;
    QString   textureFileName;
    int       ScalarSourceObjectId;

    SurfaceCuttingPlane *SurfaceCutter;
    bool CrossSectionVisible;
    PolyDataClipper *Clipper;
    int SurfaceRemovedOctantNumber;

    static vtkImageData * checkerBoardTexture;
};

ObjectSerializationHeaderMacro( PolyDataObject );

#endif
