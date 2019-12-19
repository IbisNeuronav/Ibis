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

#include "recordtrackingplugininterface.h"
#include "recordtrackingwidget.h"
#include "ui_recordtrackingwidget.h"
#include "ibisapi.h"

#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdio>
#include <unistd.h>

#include <QLabel>
#include <QCheckBox>
#include <QSpacerItem>
#include <QWidgetItem>
#include <QFileDialog>
#include <QFile>
#include <QTime>

RecordTrackingWidget::RecordTrackingWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RecordTrackingWidget),
    m_pluginInterface(nullptr),
    m_recording(false),
    m_frameId(0),
    m_cummulativeTime(0),
    m_recordfile(nullptr)
{
    m_temporaryFilename = "";
    ui->setupUi(this);
    setWindowTitle( "Rocord Tracking Information" );
}

RecordTrackingWidget::~RecordTrackingWidget()
{
    if (m_recordfile)
        m_recordfile.close();
    delete ui;
}

void RecordTrackingWidget::SetPluginInterface( RecordTrackingPluginInterface * interf )
{
    m_pluginInterface = interf;
    UpdateUi();
}

void RecordTrackingWidget::on_startButton_clicked()
{
    if ((!m_recording) && (m_pluginInterface))
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        ui->instrumentGroupBox->setEnabled(false);

        if (m_temporaryFilename != "")
        {
            m_recordfile.open(m_temporaryFilename, std::fstream::app);
            if (m_recordfile)
            {
                ui->saveButton->setEnabled(false);
                ui->startButton->setText(tr("Pause Recording"));

                m_recording = true;
                m_elapsedTimer.start();
                connect( ibisApi, SIGNAL(IbisClockTick()), this, SLOT(OnToolsPositionUpdated()) );
            }
        }
        else
        {
            m_temporaryFilename = std::tmpnam(nullptr);

            m_recordfile.open(m_temporaryFilename, std::fstream::out);
            if (m_recordfile)
            {
                m_recording = true;

                m_recordedObjectsList.clear();
                for (int i = 0; i < ui->trackedObjectLayout->count(); ++i)
                {
                    QLayoutItem * const item = ui->trackedObjectLayout->itemAt(i);
                    if(dynamic_cast<QWidgetItem *>(item))
                    {
                        QCheckBox *cb = dynamic_cast<QCheckBox *>(item->widget());
                        if(cb->isChecked())
                            m_recordedObjectsList.push_back(TrackedSceneObject::SafeDownCast( ibisApi->GetObjectByID(m_trackedObjectIds[i])) );
                    }
                }

                ui->saveButton->setEnabled(false);
                ui->startButton->setText(tr("Pause Recording"));
                m_frameId = 0;

                m_recordfile << "ObjectType = Image" << std::endl;
                m_recordfile << "NDims = 3" << std::endl;
                m_recordfile << "AnatomicalOrientation = RAS" << std::endl;
                m_recordfile << "BinaryData = True" << std::endl;
                m_recordfile << "BinaryDataByteOrderMSB = False" << std::endl;
                m_recordfile << "CenterOfRotation = 0 0 0" << std::endl;
                m_recordfile << "CompressedData = True" << std::endl;
                m_recordfile << "CompressedDataSize = 321" << std::endl;
                m_recordfile << "DimSize = ";
                m_framenumberfilep = m_recordfile.tellp();
                m_recordfile << "                         " << std::endl;
                m_recordfile << "Kinds = domain domain list" << std::endl;
                m_recordfile << "ElementSpacing = 1 1 1" << std::endl;
                m_recordfile << "ElementType = MET_UCHAR" << std::endl;
                m_recordfile << "Offset = 0 0 0" << std::endl;
                m_recordfile << "TransformMatrix = 1 0 0 0 1 0 0 0 1" << std::endl;
                m_recordfile << "UltrasoundImageOrientation = UNA" << std::endl;

                m_cummulativeTime = 0;
                m_elapsedTimer.start();
                connect( ibisApi, SIGNAL(IbisClockTick()), this, SLOT(OnToolsPositionUpdated()) );
            }
        }
    }
    else
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        disconnect( ibisApi, SIGNAL(IbisClockTick()), this, SLOT(OnToolsPositionUpdated()) );
        if (m_recordfile)
        {
            // write frame number
            m_recordfile.seekp(m_framenumberfilep);
            m_recordfile << "1 1 " << m_frameId;
            m_recordfile.close();
        }

        m_cummulativeTime += m_elapsedTimer.elapsed();
        m_recording = false;
        ui->startButton->setText(tr("Start Recording"));
        ui->saveButton->setEnabled(true);
    }

}


void RecordTrackingWidget::on_saveButton_clicked()
{
    if (m_recording)
        on_startButton_clicked();
    IbisAPI *ibisApi = m_pluginInterface->GetIbisAPI();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save trackeing sequence"),
                                                    ibisApi->GetWorkingDirectory(),
                                                    tr("Tracking sequence (*.mha)"));
    if (fileName.size() > 0)
    {
        if (!fileName.endsWith(".mha"))
            fileName += ".mha";
        if (QFile(fileName).exists())
            std::remove(fileName.toUtf8().constData());
        if (QFile::copy(tr(m_temporaryFilename.c_str()), fileName))
        {
            std::remove(m_temporaryFilename.c_str());
            m_temporaryFilename = "";
            ui->saveButton->setEnabled(false);
            ui->instrumentGroupBox->setEnabled(true);
        }
    }
}

/******************************************************************************************************/

void RecordTrackingWidget::UpdateUi()
{
    if (m_pluginInterface)
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        ibisApi->GetAllTrackedObjects(m_trackedToolsList);

        m_trackedObjectIds.clear();

        for (int i = 0; i < m_trackedToolsList.size(); ++i)
        {
            TrackedSceneObject * obj = TrackedSceneObject::SafeDownCast( m_trackedToolsList.at(i) );
            QCheckBox *checkbox = new QCheckBox;
            checkbox->setChecked(true);
            checkbox->setText( obj->GetName() );
            checkbox->setProperty("ObjectID", obj->GetObjectID());

            ui->trackedObjectLayout->addWidget(checkbox);

            m_trackedObjectIds.push_back(obj->GetObjectID());
        }
    }
}

void RecordTrackingWidget::AddTrackedTool(int objId)
{
    if(m_pluginInterface)
    {
        IbisAPI *ibisApi = m_pluginInterface->GetIbisAPI();
        SceneObject *obj = ibisApi->GetObjectByID(objId);
        if (obj)
        {
            if (obj->IsA("TrackedSceneObject"))
            {
                QCheckBox *checkbox = new QCheckBox;
                checkbox->setChecked(true);
                checkbox->setText( obj->GetName() );
                checkbox->setProperty("ObjectID", objId);

                ui->trackedObjectLayout->addWidget(checkbox);

                m_trackedObjectIds.push_back(objId);
            }
        }
    }

}

void RecordTrackingWidget::RemoveTrackedTool(int objId)
{
    for(int i = 0; i < ui->trackedObjectLayout->count(); ++i)
    {
        QWidget * w = ui->trackedObjectLayout->itemAt(i)->widget();
        if(w)
        {
            if (w->metaObject()->className() == tr("QCheckBox"))
            {
                if( objId == w->property("ObjectID").toInt() )
                {
                    std::vector<int>::iterator it = std::find(m_trackedObjectIds.begin(), m_trackedObjectIds.end(), objId);
                    if (it != m_trackedObjectIds.end())
                        m_trackedObjectIds.erase(it);
                    ui->trackedObjectLayout->removeWidget(w);
                    delete w;
                }
            }
        }
    }
}


void RecordTrackingWidget::OnToolsPositionUpdated()
{

    if (m_recordfile)
    {
        std::stringstream ss;
        ss << std::setw(10) << std::setfill('0') << m_frameId;
        std::string strFrameId = ss.str();
        m_recordfile << "Seq_Frame" + strFrameId + "_FrameNumber = " + strFrameId << std::endl;
        double timestamp = -1;

        for (size_t i = 0; i < m_recordedObjectsList.size(); ++i)
        {
            TrackedSceneObject * obj = m_recordedObjectsList.at(i);
            std::string msg;
            std::string toolStatus;

            m_recordfile << "Seq_Frame" + strFrameId + "_" + obj->GetName().toUtf8().constData() + "ToReferenceTransform = "
                         << this->GetStringFromTransform(obj->GetWorldTransform()) << std::endl;
            toolStatus = (obj->GetState() == TrackerToolState::Ok) ? "OK" : "INVALID";
            m_recordfile << "Seq_Frame" + strFrameId + "_" + obj->GetName().toUtf8().constData() + "ToReferenceTransformStatus = "
                         << toolStatus << std::endl;
            if (timestamp < 0)
                timestamp = obj->GetLastTimestamp();
        }

        m_recordfile << "Seq_Frame" + strFrameId + "_Timestamp = " << std::fixed << timestamp << std::endl;
        m_recordfile << "Seq_Frame" + strFrameId + "_UnfilteredTimestamp = " << std::fixed << timestamp << std::endl;
        //m_recordfile << "Seq_Frame" + strFrameId + "_ImageStatus = INVALID" << std::endl;
        m_frameId++;

        QTime time(0,0,0);
        time = time.addMSecs(m_elapsedTimer.elapsed() + m_cummulativeTime);
        ui->numberOfFramesLabel->setText("Number of frames: " + QString::number(m_frameId));
        ui->elapsedTimeLabel->setText("Time: " + time.toString("hh:mm:ss"));
    }
}

std::string RecordTrackingWidget::GetStringFromTransform(vtkTransform * transform)
{
    std::stringstream strTransform;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            double elm = transform->GetMatrix()->GetElement(i,j);
            strTransform << elm << " ";
        }
    }
    return strTransform.str();
}
