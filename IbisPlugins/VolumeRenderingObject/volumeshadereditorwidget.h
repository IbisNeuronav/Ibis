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

#ifndef VOLUMESHADEREDITORWIDGET_H
#define VOLUMESHADEREDITORWIDGET_H

#include <QWidget>

class VolumeRenderingObject;

namespace Ui {
class VolumeShaderEditorWidget;
}

class VolumeShaderEditorWidget : public QWidget
{

    Q_OBJECT
    
public:

    explicit VolumeShaderEditorWidget(QWidget *parent = 0);
    ~VolumeShaderEditorWidget();

    void SetVolumeRenderer( VolumeRenderingObject * ren, int volumeIndex );
    
private slots:

    void VolumeRendererModifiedSlot();
    void on_applyButton_clicked();
    void on_revertButton_clicked();

    void on_duplicateShaderButton_clicked();

private:

    void SetShaderCode( QString shaderCode );
    QString GetShaderCode();
    bool IsShaderCustom();
    void DuplicateShader();
    void UpdateUi();
    QString GetCurrentShaderTypeName();

    VolumeRenderingObject * m_volumeRenderer;
    int m_volumeIndex;
    QString m_backupShaderTypeName;
    QString m_backupShaderCode;
    Ui::VolumeShaderEditorWidget * ui;
};

#endif
