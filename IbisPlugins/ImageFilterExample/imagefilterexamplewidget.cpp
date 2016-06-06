/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#include "imagefilterexamplewidget.h"
#include "ui_imagefilterexamplewidget.h"
#include "application.h"
#include "scenemanager.h"
#include "sceneobject.h"
#include "imageobject.h"
#include "vtkTransform.h"
#include <QComboBox>
#include <QMessageBox>
#include <unistd.h>

ImageFilterExampleWidget::ImageFilterExampleWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImageFilterExampleWidget),
    m_application(0)
{
    ui->setupUi(this);
    setWindowTitle( "Image Filter Example" );
    UpdateUi();
}

ImageFilterExampleWidget::~ImageFilterExampleWidget()
{
    delete ui;
}

void ImageFilterExampleWidget::SetApplication( Application * app )
{
    m_application = app;
    UpdateUi();
}

void ImageFilterExampleWidget::on_startButton_clicked()
{
    // Make sure all params have been specified
    int transformObjectId = ui->transformObjectComboBox->itemData( ui->transformObjectComboBox->currentIndex() ).toInt();
    int sourceImageObjectId = ui->sourceImageComboBox->itemData( ui->sourceImageComboBox->currentIndex() ).toInt();
    int targetImageObjectId = ui->targetImageComboBox->itemData( ui->targetImageComboBox->currentIndex() ).toInt();

    if( transformObjectId == -1 || sourceImageObjectId == -1 || targetImageObjectId == -1 )
    {
        QMessageBox::information( this, "Image Filter Example", "Need to specify source, target and affected transform before processing" );
        return;
    }

    // Get input images
    SceneManager * sm = m_application->GetSceneManager();
    ImageObject * sourceImageObject = ImageObject::SafeDownCast( sm->GetObjectByID( sourceImageObjectId ) );
    Q_ASSERT_X( sourceImageObject, "ImageFilterExampleWidget::on_startButton_clicked()", "Invalid source object" );
    ImageObject * targetImageObject = ImageObject::SafeDownCast( sm->GetObjectByID( targetImageObjectId ) );
    Q_ASSERT_X( targetImageObject, "ImageFilterExampleWidget::on_startButton_clicked()", "Invalid target object" );

    IbisItk3DImageType::Pointer itkSourceImage = sourceImageObject->GetItkImage();
    IbisItk3DImageType::Pointer itkTargetImage = targetImageObject->GetItkImage();

    //====================================================================================
    // -----------------------     TODO    ---------------------------------------
    // We have pointers to source and target images, now do your processing here...
    //====================================================================================

    ui->progressBar->setEnabled( true );
    ui->startButton->setEnabled( false );

    // TODO : artificial update code, modify this in a real plugin to reflect reality.
    for( int i = 0; i < 100; ++i )
    {
        ui->progressBar->setValue( i );
        QCoreApplication::processEvents();
        usleep( 50000 ); // sleep 50 ms
    }

    ui->startButton->setEnabled( true );
    ui->progressBar->setValue( 0 );
    ui->progressBar->setEnabled( false );

    SceneObject * transformObject = sm->GetObjectByID( transformObjectId );
    vtkTransform * transform = vtkTransform::SafeDownCast( transformObject->GetLocalTransform() );
    Q_ASSERT_X( transform, "ImageFilterExampleWidget::on_startButton_clicked()", "Invalid transform" );

    transformObject->StartModifyingTransform();

    //====================================================================================
    // -----------------------     TODO    ---------------------------------------
    // Apply desired changes to the target transform.
    // transformObject->FinishModifyingTransform() will make sure changes you made to
    // the transform are reflected in the graphic windows and UI.
    //====================================================================================

    transformObject->FinishModifyingTransform();

}

void ImageFilterExampleWidget::UpdateUi()
{
    ui->transformObjectComboBox->clear();
    ui->sourceImageComboBox->clear();
    ui->targetImageComboBox->clear();
    if( m_application )
    {
        SceneManager * sm = m_application->GetSceneManager();
        const SceneManager::ObjectList & allObjects = sm->GetAllObjects();
        for( int i = 0; i < allObjects.size(); ++i )
        {
            SceneObject * current = allObjects[i];
            if( current != sm->GetSceneRoot() && current->IsListable() )
            {
                vtkTransform * localTransform = vtkTransform::SafeDownCast( current->GetLocalTransform() );
                if( localTransform && current->CanEditTransformManually() )
                    ui->transformObjectComboBox->addItem( current->GetName(), QVariant( current->GetObjectID() ) );

                if( current->IsA("ImageObject") )
                {
                    ui->sourceImageComboBox->addItem( current->GetName(), QVariant( current->GetObjectID() ) );
                    ui->targetImageComboBox->addItem( current->GetName(), QVariant( current->GetObjectID() ) );
                }
            }
        }

        if( ui->sourceImageComboBox->count() == 0 )
        {
            ui->sourceImageComboBox->addItem( "None", QVariant(-1) );
        }
        if( ui->targetImageComboBox->count() == 0 )
        {
            ui->targetImageComboBox->addItem( "None", QVariant(-1) );
        }
    }
}
