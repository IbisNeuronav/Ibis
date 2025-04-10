/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef POINTCLOUDOBJECT_H
#define POINTCLOUDOBJECT_H

#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkSmartPointer.h>
#include <vtkVertexGlyphFilter.h>

#include <QObject>
#include <QVector>
#include <map>

#include "sceneobject.h"
#include "serializer.h"

class PointCloudObject : public SceneObject
{
    Q_OBJECT

public:
    static PointCloudObject * New() { return new PointCloudObject; }
    vtkTypeMacro( PointCloudObject, SceneObject );

    PointCloudObject();
    virtual ~PointCloudObject();

    virtual void Serialize( Serializer * ser ) override;
    virtual void Export() override;
    virtual bool IsExportable() override { return true; }

    // Implementation of parent virtual method
    virtual void Setup( View * view ) override;
    virtual void Release( View * view ) override;

    virtual void Hide() override;
    virtual void Show() override;

    virtual void CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets ) override;

    double GetOpacity() { return this->m_Opacity; }
    void SetOpacity( double opacity );
    void SetColor( double color[3] );
    void UpdateSettingsWidget();
    void SetProperty( vtkProperty * property );
    vtkGetObjectMacro( Property, vtkProperty );

    void SetPointCloudArray( vtkPoints * pointCloudArray );

signals:

    void ObjectViewChanged();

protected:
    void UpdateMapperPipeline();
    void ChooseLookupTable();
    virtual void InternalPostSceneRead() override;

    vtkProperty * Property;
    vtkSmartPointer<vtkPoints> m_PointCloudArray;
    vtkSmartPointer<vtkPolyData> m_PointsPolydata;
    vtkSmartPointer<vtkVertexGlyphFilter> m_PointCloudGlyphFilter;

    typedef std::map<View *, vtkActor *> PointCloudObjectViewAssociationType;
    PointCloudObjectViewAssociationType m_PointCloudObjectInstances;

    double m_Opacity;  // between 0 and 1
    double m_Color[3];
};

ObjectSerializationHeaderMacro( PointCloudObject );

#endif
