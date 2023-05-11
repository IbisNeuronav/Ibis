/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TRANSFORMEDITWIDGET_H
#define TRANSFORMEDITWIDGET_H

#include <QObject>
#include <QWidget>

class vtkEventQtSlotConnect;
class vtkQtMatrixDialog;
class vtkMatrix4x4;
class SceneObject;

namespace Ui
{
class TransformEditWidget;
}

class TransformEditWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TransformEditWidget( QWidget * parent = 0 );
    ~TransformEditWidget();

    void SetSceneObject( SceneObject * obj );

protected:
    void EnableSpinBoxes( bool on );
    void BlockSpinboxSignals( bool block );

private:
    Ui::TransformEditWidget * ui;
    SceneObject * m_sceneObject;
    vtkEventQtSlotConnect * m_transformModifiedConnection;
    bool m_selfUpdating;
    vtkQtMatrixDialog * m_matrixDialog;
    vtkQtMatrixDialog * m_worldMatrixDialog;

public slots:
    void UpdateUi();  // take data in transform and put it in ui
    void TransformModified( vtkMatrix4x4 * mat );

private slots:

    void UpdateTransform();  // take data in ui and put it in transform
    void EditMatrixButtonToggled( bool isOn );
    void EditMatrixDialogClosed();
    void WorldMatrixButtonToggled( bool isOn );
    void WorldMatrixDialogClosed();
    void SetIdentityButtonClicked();
};

#endif  // TRANSFORMEDITWIDGET_H
