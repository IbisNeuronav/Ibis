/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <QFileDialog>
#include <QMessageBox>
#include "pointcloudobject.h"
#include "vtkRenderer.h"
#include "view.h"
#include "vtkTransform.h"
#include "vtkAssembly.h"
#include "vtkProperty.h"
#include "vtkPolyDataWriter.h"

#include "pointcloudobjectsettingsdialog.h"
#include "application.h"
#include "scenemanager.h"
#include "imageobject.h"
#include "vtkProbeFilter.h"

vtkCxxSetObjectMacro(PointCloudObject, Property, vtkProperty);
ObjectSerializationMacro( PointCloudObject );


PointCloudObject::PointCloudObject()
{

    this->Property = vtkProperty::New();
    this->m_Opacity = 1.0;
    for (int i = 0; i< 3; i++)
        this->m_Color[i] = 1.0;

    this->m_PointCloudArray =  vtkSmartPointer<vtkPoints>::New();
    this->m_PointsPolydata = vtkSmartPointer<vtkPolyData>::New();
    this->m_PointsPolydata->SetPoints(m_PointCloudArray);

    this->m_PointCloudGlyphFilter = vtkVertexGlyphFilter::New();
    this->m_PointCloudGlyphFilter->SetInputData(m_PointsPolydata);


}

PointCloudObject::~PointCloudObject()
{
    this->Property->Delete();
    this->m_PointCloudArray = 0;
    this->m_PointsPolydata = 0;
    this->m_PointCloudGlyphFilter = 0;
}


void PointCloudObject::Serialize( Serializer * ser )
{
    SceneObject::Serialize(ser);
    ::Serialize( ser, "Opacity", this->m_Opacity );
    ::Serialize( ser, "ObjectColor", this->m_Color, 3 );
    if (ser->IsReader())
    {
        this->SetColor(this->m_Color);
        this->SetOpacity(this->m_Opacity);
        this->UpdateSettingsWidget();
        emit Modified();
    }
}

void PointCloudObject::Export()
{
    Q_ASSERT( this->GetManager() );

    QString surfaceName(this->Name);
    surfaceName.append(".vtk");
    this->SetDataFileName(surfaceName);
    QString fullName(this->GetManager()->GetSceneDirectory());
    fullName.append("/");
    fullName.append(surfaceName);
    QString saveName = Application::GetInstance().GetSaveFileName( tr("Save Object"), fullName, tr("*.vtk") );
    if(saveName.isEmpty())
        return;
    if (QFile::exists(saveName))
    {
        int ret = QMessageBox::warning(0, tr("Save PointCloudObject"), saveName,
                QMessageBox::Yes | QMessageBox::Default,
                QMessageBox::No | QMessageBox::Escape);
        if (ret == QMessageBox::No)
            return;
    }
    this->SetFullFileName(saveName);
    vtkPolyDataWriter *writer1 = vtkPolyDataWriter::New();
    writer1->SetFileName( saveName.toUtf8().data() );
    writer1->SetInputData(this->m_PointsPolydata);
    writer1->Update();
    writer1->Write();
    writer1->Delete();
}

void PointCloudObject::Setup( View * view )
{
    SceneObject::Setup( view );

    if( view->GetType() == THREED_VIEW_TYPE )
    {
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);

        actor->SetProperty(this->Property);
        view->GetRenderer()->AddActor(actor);
        this->m_PointCloudObjectInstances[ view ] = actor;

        actor->GetMapper()->SetInputConnection(m_PointCloudGlyphFilter->GetOutputPort());

        connect( this, SIGNAL( Modified() ), view, SLOT( NotifyNeedRender() ) );
        this->GetProperty()->GetColor(this->m_Color);

    }
}

// Deep Copy of Point Cloud Array
void PointCloudObject::SetPointCloudArray(vtkPoints * pointCloudArray)
{
    for(  size_t i=0; i < pointCloudArray->GetNumberOfPoints(); i++ )
    {
        m_PointCloudArray->InsertNextPoint (pointCloudArray->GetPoint(i));
    }
    this->m_PointsPolydata->SetPoints(m_PointCloudArray);
    emit Modified();

}

void PointCloudObject::SetColor(double color[3])
{
    for (int i = 0; i < 3; i++)
        this->m_Color[i] = color[i];
    this->Property->SetColor(this->m_Color);
    emit Modified();
}

void PointCloudObject::Release( View * view )
{
    SceneObject::Release( view );

    if( view->GetType() == THREED_VIEW_TYPE )
    {
        this->disconnect( view );

        PointCloudObjectViewAssociationType::iterator itAssociations = this->m_PointCloudObjectInstances.find( view );

        if( itAssociations != this->m_PointCloudObjectInstances.end() )
        {
            vtkSmartPointer<vtkActor> actor = (*itAssociations).second;
            view->GetRenderer()->RemoveViewProp( actor );
            this->m_PointCloudObjectInstances.erase( itAssociations );
            disconnect( view, SLOT(NotifyNeedRender()) );
        }
    }
}

void PointCloudObject::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets)
{
    PointCloudObjectSettingsDialog * res = new PointCloudObjectSettingsDialog( parent );
    res->setAttribute(Qt::WA_DeleteOnClose);
    res->SetPointCloudObject( this );
    res->setObjectName("Properties");
    connect( this, SIGNAL( ObjectViewChanged() ), res, SLOT(UpdateSettings()) );
    widgets->append(res);
}

void PointCloudObject::SetOpacity( double opacity )
{
    if( opacity > 1 )
        this->m_Opacity = 1;
    else if( opacity < 0 )
        this->m_Opacity = 0;
    else
        this->m_Opacity = opacity;

    this->Property->SetOpacity( this->m_Opacity );

    emit Modified();
}

void PointCloudObject::Hide()
{
    View * view = 0;
    vtkActor * actor = 0;
    PointCloudObjectViewAssociationType::iterator it = this->m_PointCloudObjectInstances.begin();
    for( ; it != this->m_PointCloudObjectInstances.end(); ++it )
    {
        view = (*it).first;
        if (view->GetType() == THREED_VIEW_TYPE)
        {
            actor = (*it).second;
            actor->VisibilityOff();
        }
    }
}

void PointCloudObject::Show()
{
    View * view = 0;
    vtkActor * actor = 0;
    PointCloudObjectViewAssociationType::iterator it = this->m_PointCloudObjectInstances.begin();
    for( ; it != this->m_PointCloudObjectInstances.end(); ++it )
    {
        view = (*it).first;
        if (view->GetType() == THREED_VIEW_TYPE)
        {
            actor = (*it).second;
            actor->VisibilityOn();
            break;
        }
    }
}

void PointCloudObject::UpdateSettingsWidget()
{
    emit ObjectViewChanged();
}

void PointCloudObject::InternalPostSceneRead()
{
    emit ObjectViewChanged();
}
