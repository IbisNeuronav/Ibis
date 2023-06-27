/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef LANDMARKREGISTRATIONOBJECTSETTINGSWIDGET_H
#define LANDMARKREGISTRATIONOBJECTSETTINGSWIDGET_H

#include <QObject>
#include <QStandardItemModel>
#include <QString>
#include <QWidget>

#include "landmarkregistrationobject.h"

class Application;

namespace Ui
{
class LandmarkRegistrationObjectSettingsWidget;
}

class LandmarkRegistrationObjectSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LandmarkRegistrationObjectSettingsWidget( QWidget * parent = 0 );
    ~LandmarkRegistrationObjectSettingsWidget();

    void SetLandmarkRegistrationObject( LandmarkRegistrationObject * obj );
    bool ReadTagFile();
    void SetApplication( Application * app );

public slots:

    void UpdateUI();
    void PointDataChanged( QStandardItem * );

private slots:

    void on_registerPushButton_toggled( bool checked );
    virtual void on_capturePushButton_clicked();
    virtual void on_importPushButton_clicked();
    virtual void on_detailsPushButton_clicked();
    virtual void on_pointsTreeView_clicked( QModelIndex idx );
    virtual void on_targetComboBox_currentIndexChanged( int index );
    virtual void EnableDisablePoint();
    virtual void DeletePoint();
    virtual void UpdateCaptureButton();

protected:
    LandmarkRegistrationObject * m_registrationObject;
    void contextMenuEvent( QContextMenuEvent * event );

private:
    void SetCaptureButtonBackgroundColor( QColor col );

    Application * m_application;
    QStandardItemModel * m_model;
    QColor m_capture_button_color;

    Ui::LandmarkRegistrationObjectSettingsWidget * ui;
};

#endif  // LANDMARKREGISTRATIONOBJECTSETTINGSWIDGET_H
