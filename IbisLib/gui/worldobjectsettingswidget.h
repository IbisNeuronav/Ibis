/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef WORLDOBJECTSETTINGSWIDGET_H
#define WORLDOBJECTSETTINGSWIDGET_H

#include <QObject>
#include <QWidget>

namespace Ui
{
class WorldObjectSettingsWidget;
}

class WorldObject;

class WorldObjectSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WorldObjectSettingsWidget( QWidget * parent = 0 );
    ~WorldObjectSettingsWidget();

    void SetWorldObject( WorldObject * obj );

public slots:

    void On3DInteractionButtonClicked( int );

private slots:

    void on_updateMaxFrequencySlider_valueChanged( int value );
    void on_viewFollowsReferenceCheckBox_toggled( bool checked );
    void on_cameraAngleSlider_valueChanged( int value );
    void on_cameraAngleSpinBox_valueChanged( int arg1 );
    void on_changeColorButton_clicked();
    void on_change3DColorButton_clicked();
    void on_changeCursorColorButton_clicked();
    void on_showAxesCheckBox_toggled( bool );
    void on_showCursorCheckBox_toggled( bool );

private:
    void UpdateUi();
    int GetUpdateFrequencyIndex( double fps );

    WorldObject * m_worldObject;

    Ui::WorldObjectSettingsWidget * ui;
};

#endif
