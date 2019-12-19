/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Houssem Gueziri for writing this class

#ifndef __RecordTrackingWidget_h_
#define __RecordTrackingWidget_h_

#include <QWidget>
#include <QElapsedTimer>

#include <fstream>
#include "trackedsceneobject.h"
#include <vtkTransform.h>
#include <vector>
#include <iostream>

class RecordTrackingPluginInterface;

namespace Ui
{
    class RecordTrackingWidget;
}


class RecordTrackingWidget : public QWidget
{

    Q_OBJECT

public:

    explicit RecordTrackingWidget(QWidget *parent = nullptr);
    ~RecordTrackingWidget();

    void AddTrackedTool(int);
    void RemoveTrackedTool(int);

    void SetPluginInterface( RecordTrackingPluginInterface * interf );

private:
    std::string GetStringFromTransform(vtkTransform *);

private:

    void UpdateUi();

    Ui::RecordTrackingWidget * ui;
    RecordTrackingPluginInterface * m_pluginInterface;

    bool m_recording;
    std::ofstream m_recordfile;
    QList<TrackedSceneObject *> m_trackedToolsList;
    unsigned long int m_frameId;
    std::vector<int> m_trackedObjectIds;
    std::vector<TrackedSceneObject *> m_recordedObjectsList;
    std::iostream::pos_type m_framenumberfilep;
    std::string m_temporaryFilename;
    QElapsedTimer m_elapsedTimer;
    qint64 m_cummulativeTime;


private slots:

    void on_startButton_clicked();
    void on_saveButton_clicked();
    void OnToolsPositionUpdated();

};

#endif
