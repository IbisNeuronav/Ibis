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

#include <map>
#include <QVector>
#include <vtkSmartPointer.h>

#include "sceneobject.h"
#include "serializer.h"
#include "abstractpolydataobject.h"

class vtkDataSetAlgorithm;
class vtkImageData;
class vtkProbeFilter;
class vtkScalarsToColors;
class ImageObject;

/**
 * @class   PolyDataObject
 * @brief   PolyDataObject is derived from AbstractPolyDataObject
 *
 * PolyDataObject provides a settings widget allowing you to set object's properties.
 * It may be exported as a *.vtk type file.
 *
 *  @sa SceneObject SceneManager AbstractPolyDataObject
 */

class PolyDataObject : public AbstractPolyDataObject
{
    
Q_OBJECT

public:
        
    static PolyDataObject * New() { return new PolyDataObject; }
    vtkTypeMacro(PolyDataObject,AbstractPolyDataObject);
    
    PolyDataObject();
    virtual ~PolyDataObject();
    virtual void Serialize( Serializer * ser ) override;
    virtual void Export() override;
    virtual bool IsExportable()  override { return true; }
    
    virtual void UpdatePipeline() override;
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets) override;


    ImageObject * GetScalarSource() { return this->ScalarSource; }
    void SetScalarSource( ImageObject * im );
    int GetLutIndex() { return LutIndex; }
    void SetLutIndex( int index );
    int GetVertexColorMode() { return VertexColorMode; }
    void SetVertexColorMode( int mode );
    vtkScalarsToColors * GetCurrentLut();
    virtual void InternalPostSceneRead() override;

    void SetTexture( vtkImageData * texImage );
    bool GetShowTexture() { return showTexture; }
    void SetShowTexture( bool show );
    void SetTextureFileName( QString );
    QString GetTextureFileName() { return textureFileName; }

public slots:

    void OnScalarSourceDeleted();
    void OnScalarSourceModified();

protected:

    vtkSmartPointer<vtkScalarsToColors> CurrentLut;
    ImageObject * ScalarSource;
    vtkSmartPointer<vtkScalarsToColors> LutBackup;
    vtkSmartPointer<vtkProbeFilter> ProbeFilter;

    int VertexColorMode;   // 0 : use scalars in data, 1 : get scalars from object ScalarSourceObjectId
    int ScalarSourceObjectId;
    int LutIndex;

    vtkImageData* Texture;
    vtkSmartPointer<vtkDataSetAlgorithm> TextureMap;
    bool      showTexture;
    QString   textureFileName;
    static vtkSmartPointer<vtkImageData> checkerBoardTexture;
};

ObjectSerializationHeaderMacro( PolyDataObject );

#endif
