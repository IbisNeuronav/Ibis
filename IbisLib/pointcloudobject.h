/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __PointCloudObject_h_
#define __PointCloudObject_h_

#include "sceneobject.h"
#include "serializer.h"
#include <map>
#include <QVector>

#include "vtkSmartPointer.h"
#include "vtkPolyDataMapper.h"
#include "vtkVertexGlyphFilter.h"
#include "vtkProperty.h"

//class vtkPoints;
//class vtkPolyData;
//class vtkTransform;
//class vtkActor;
//class vtkProperty;
//class vtkVertexGlyphFilter;


class PointCloudObject : public SceneObject
{

Q_OBJECT

public:

    static PointCloudObject * New() { return new PointCloudObject; }
    vtkTypeMacro(PointCloudObject,SceneObject);

    PointCloudObject();
    virtual ~PointCloudObject();

    virtual void Serialize( Serializer * ser );
    virtual void Export();
    virtual bool IsExportable()  { return true; }

    // Implementation of parent virtual method
    virtual void Setup( View * view );
    virtual void Release( View * view );

    virtual void Hide();
    virtual void Show();

    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets);

    double GetOpacity() { return this->m_Opacity; }
    void SetOpacity( double opacity );
    void SetColor(double color[3]);
    void UpdateSettingsWidget();
    void SetProperty( vtkProperty * property );
    vtkGetObjectMacro( Property, vtkProperty );

    void SetPointCloudArray( vtkPoints * pointCloudArray );


signals:

    void ObjectViewChanged();

protected:

    void UpdateMapperPipeline();
    void ChooseLookupTable();
    virtual void InternalPostSceneRead();

    vtkProperty *  Property;
    vtkSmartPointer<vtkPoints>   m_PointCloudArray;
    vtkSmartPointer<vtkPolyData> m_PointsPolydata;
    vtkSmartPointer<vtkVertexGlyphFilter> m_PointCloudGlyphFilter;

    typedef std::map<View*,vtkActor*>   PointCloudObjectViewAssociationType;
    PointCloudObjectViewAssociationType m_PointCloudObjectInstances;

    double    m_Opacity;        // between 0 and 1
    double    m_Color[3];


};

ObjectSerializationHeaderMacro( PointCloudObject );

#endif
