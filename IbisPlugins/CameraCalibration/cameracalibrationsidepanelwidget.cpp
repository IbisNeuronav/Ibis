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

#include "cameracalibrationsidepanelwidget.h"
#include "ui_cameracalibrationsidepanelwidget.h"
#include "cameracalibrationplugininterface.h"
#include "cameracalibrator.h"
#include "ibisapi.h"
#include "cameraobject.h"
#include "vtkMatrix4x4Operators.h"
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>

CameraCalibrationSidePanelWidget::CameraCalibrationSidePanelWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CameraCalibrationSidePanelWidget)
{
    ui->setupUi(this);
    m_pluginInterface = 0;
}

CameraCalibrationSidePanelWidget::~CameraCalibrationSidePanelWidget()
{
    delete ui;
}

void CameraCalibrationSidePanelWidget::SetPluginInterface( CameraCalibrationPluginInterface * pluginInterface )
{
    Q_ASSERT( m_pluginInterface == 0 );
    Q_ASSERT( pluginInterface != 0 );

    m_pluginInterface = pluginInterface;

    connect( m_pluginInterface, SIGNAL(PluginModified()), this, SLOT(InterfaceModified()) );
    connect( m_pluginInterface, SIGNAL(CameraCalibrationWidgetClosedSignal()), this, SLOT(UpdateUi()) );

    UpdateUi();
}

void CameraCalibrationSidePanelWidget::InterfaceModified()
{
    UpdateUi();
}

void CameraCalibrationSidePanelWidget::UpdateUi()
{
    Q_ASSERT( m_pluginInterface );

    // Update current camera combo box
    ui->currentCameraComboBox->blockSignals( true );
    ui->currentCameraComboBox->clear();
    QList<CameraObject*> allCams;
    m_pluginInterface->GetIbisAPI()->GetAllCameraObjects( allCams );
    int nbValidCams = 0;
    for( int i = 0; i < allCams.size(); ++i )
    {
        ui->currentCameraComboBox->addItem( allCams[i]->GetName(), QVariant( allCams[i]->GetObjectID() ) );
        if( allCams[i]->GetObjectID() == m_pluginInterface->GetCurrentCameraObjectId() )
            ui->currentCameraComboBox->setCurrentIndex( i );
    }
    if( allCams.size() == 0 )
        ui->currentCameraComboBox->addItem( "None", QVariant( IbisAPI::InvalidId ) );
    ui->currentCameraComboBox->blockSignals( false );

    ui->gridWidthSpinBox->blockSignals( true );
    ui->gridWidthSpinBox->setValue( m_pluginInterface->GetCalibrationGridWidth() );
    ui->gridWidthSpinBox->blockSignals( false );

    ui->gridHeightSpinBox->blockSignals( true );
    ui->gridHeightSpinBox->setValue( m_pluginInterface->GetCalibrationGridHeight() );
    ui->gridHeightSpinBox->blockSignals( false );

    ui->gridSquareSizeSpinBox->blockSignals( true );
    ui->gridSquareSizeSpinBox->setValue( m_pluginInterface->GetCalibrationGridCellSize() );
    ui->gridSquareSizeSpinBox->blockSignals( false );

    ui->showGridCheckBox->blockSignals( true );
    ui->showGridCheckBox->setChecked( m_pluginInterface->IsShowingGrid() );
    ui->showGridCheckBox->blockSignals( false );

    ui->translationScaleSpinBox->blockSignals( true );
    ui->translationScaleSpinBox->setValue( m_pluginInterface->GetExtrinsicTranslationScale() );
    ui->translationScaleSpinBox->blockSignals( false );

    ui->rotationScaleSpinBox->blockSignals( true );
    ui->rotationScaleSpinBox->setValue( m_pluginInterface->GetExtrinsicRotationScale() );
    ui->rotationScaleSpinBox->blockSignals( false );

    ui->extrinsicReprojErrorLabel->setText( QString( "Avg. extrinsic reproj err.: %1").arg( m_pluginInterface->GetMeanRoprojectionError() ));

    ui->calibrateButton->blockSignals( true );
    ui->calibrateButton->setEnabled( m_pluginInterface->HasValidCamera() );
    ui->calibrateButton->setChecked( m_pluginInterface->IsCalibrationWidgetOn() );
    ui->calibrateButton->blockSignals( false );

    // list all views
    ui->viewListWidget->blockSignals( true );
    ui->viewListWidget->clear();
    CameraCalibrator * calib = m_pluginInterface->GetCameraCalibrator();
    for( int i = 0; i < calib->GetNumberOfViews(); ++i )
    {
        QString itemName = QString( "V%1 - %2" ).arg( i ).arg( calib->GetViewReprojectionError( i ) );
        QListWidgetItem * item = new QListWidgetItem( itemName, ui->viewListWidget );
        if( calib->IsViewEnabled( i ) )
            item->setCheckState( Qt::Checked );
        else
            item->setCheckState( Qt::Unchecked );
    }
    ui->viewListWidget->blockSignals( false );
}

void CameraCalibrationSidePanelWidget::on_gridWidthSpinBox_valueChanged(int arg1)
{
    m_pluginInterface->SetCalibrationGridWidth( arg1 );
}

void CameraCalibrationSidePanelWidget::on_gridHeightSpinBox_valueChanged(int arg1)
{
    m_pluginInterface->SetCalibrationGridHeight( arg1 );
}

void CameraCalibrationSidePanelWidget::on_gridSquareSizeSpinBox_valueChanged(int arg1)
{
    m_pluginInterface->SetCalibrationGridSquareSize( (double)arg1 );
}

void CameraCalibrationSidePanelWidget::on_calibrateButton_toggled( bool checked )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->StartCalibrationWidget( checked );
}

void CameraCalibrationSidePanelWidget::on_currentCameraComboBox_currentIndexChanged( int index )
{
    Q_ASSERT( m_pluginInterface );
    QVariant v = ui->currentCameraComboBox->itemData( index );
    bool ok = false;
    int objectId = v.toInt( &ok );
    Q_ASSERT( ok );
    m_pluginInterface->SetCurrentCameraObjectId( objectId );
}

void CameraCalibrationSidePanelWidget::on_exportCalibrationDataButton_clicked()
{
    Q_ASSERT( m_pluginInterface );

    QString dir = QFileDialog::getExistingDirectory( this, "Choose directory to export calibration data", QDir::homePath() );
    if( !dir.isEmpty() )
    {
        QProgressDialog * dlg = m_pluginInterface->GetIbisAPI()->StartProgress( 100, "Exporting calibration data..." );
        m_pluginInterface->GetCameraCalibrator()->ExportCalibrationData( dir, dlg );
        m_pluginInterface->GetIbisAPI()->StopProgress( dlg );
    }
}

void CameraCalibrationSidePanelWidget::on_importDataButton_clicked()
{
    Q_ASSERT( m_pluginInterface );

    QString dir = QFileDialog::getExistingDirectory( this, "Choose directory where calibration data is located", QDir::homePath() );
    if( !dir.isEmpty() )
    {
        m_pluginInterface->ImportCalibrationData( dir );
    }
}

void CameraCalibrationSidePanelWidget::on_showGridCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->ShowGrid( checked );
}

void CameraCalibrationSidePanelWidget::on_showAllCapturedViewsButton_clicked()
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->ShowAllCapturedViews();
}

void CameraCalibrationSidePanelWidget::on_hideAllCapturedViewsButton_clicked()
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->HideAllCapturedViews();
}

void CameraCalibrationSidePanelWidget::on_validationButton_clicked()
{
    Q_ASSERT( m_pluginInterface );

    QProgressDialog * dlg = m_pluginInterface->GetIbisAPI()->StartProgress( 100, "Computing cross-validation..." );
    double tScale = m_pluginInterface->GetExtrinsicTranslationScale();
    double rScale = m_pluginInterface->GetExtrinsicRotationScale();
    double stdDevReprojError = 0.0;
    double minDist = 0.0;
    double maxDist = 0.0;
    double avgReprojError = m_pluginInterface->GetCameraCalibrator()->ComputeCrossValidation( tScale, rScale, stdDevReprojError, minDist, maxDist, dlg );
    m_pluginInterface->GetIbisAPI()->StopProgress( dlg );

    QString validationText = QString("Average reprojection error: %1 +/- %2 mm\n").arg( avgReprojError ).arg(stdDevReprojError);
    validationText += QString("Distance range: ( %1, %2 )").arg( minDist ).arg( maxDist );
    QMessageBox::information( this, "Calibration validation", validationText, QMessageBox::Ok );
}

void CameraCalibrationSidePanelWidget::on_translationScaleSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetExtrinsicTranslationScale( arg1 );
}

void CameraCalibrationSidePanelWidget::on_rotationScaleSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetExtrinsicRotationScale( arg1 );
}

void CameraCalibrationSidePanelWidget::on_updateCalibrationButton_clicked()
{
    m_pluginInterface->DoCalibration();
}

void CameraCalibrationSidePanelWidget::on_viewListWidget_itemChanged( QListWidgetItem * item )
{
    int index = ui->viewListWidget->row( item );
    bool enabled = item->checkState() == Qt::Checked;
    m_pluginInterface->GetCameraCalibrator()->SetViewEnabled( index, enabled );
    m_pluginInterface->DoCalibration();
}
