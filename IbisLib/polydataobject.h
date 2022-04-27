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
 * PolyDataObject provides a mechanism allowing you to set object properties.
 * PolyDataObject may be exported as a *.vtk type file.
 *
 *  @sa SceneObject SceneManager ImageObject LookupTableManager AbstractPolyDataObject PolyDataObjectSettingsDialog
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
    /** Update clipping, colors, visibility. */
    virtual void UpdatePipeline() override;
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets) override;

    /** Get current scalar source */
    ImageObject * GetScalarSource() { return this->ScalarSource; }
    /** Set current scalar source */
    void SetScalarSource( ImageObject * im );
    /** Get index to the current lookup table, lookup tables are defined in LookupTableManager. */
    int GetLutIndex() { return LutIndex; }
    /** LUT index is set when Vertex Color is checked and Use Scalars is selected in PolyDataObjectSettingsDialog. */
    void SetLutIndex( int index );
    /** Check if vertex color mode is used, */
    int GetVertexColorMode() { return VertexColorMode; }
    /** Set vertex color mode. */
    void SetVertexColorMode( int mode );
    /** Get current lookup table. */
    vtkScalarsToColors * GetCurrentLut();
    virtual void InternalPostSceneRead() override;
    /**  Set image found in texture file as PolyData texture. */
    void SetTexture( vtkImageData * texImage );
    /** Check if the texture is displayed. */
    bool GetShowTexture() { return showTexture; }
    /** Show/hide texture. */
    void SetShowTexture( bool show );
    /** Set the name of the file used for texture - .png format is used. */
    void SetTextureFileName( QString );
    /**  Get the name of the file used for texture. */
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
