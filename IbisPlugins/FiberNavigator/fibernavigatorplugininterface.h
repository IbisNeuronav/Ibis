/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Francois Rheau for writing this class

#ifndef __FiberNavigatorPluginInterface_h_
#define __FiberNavigatorPluginInterface_h_

#include <cmath>

#include <QFileDialog>
#include <QList>
#include <QtPlugin>
#include <QString>
#include <QtGui>

#include "fiberobjectsettingswidget.h"
#include "fractionalAnisotropy.h"
#include "imageobject.h"
#include "maximas.h"
#include "polydataobject.h"
#include "view.h"
#include "SVL.h"
#include "toolplugininterface.h"

#include <vtkAbstractTransform.h>
#include <vtkActor.h>
#include <vtkBoxWidget2.h>
#include <vtkCellArray.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkImageData.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransformFilter.h>
#include <vtkTubeFilter.h>
#include <vtkRenderer.h>
#include <vtkUnsignedCharArray.h>
#include <vtkWidgetRepresentation.h>

typedef QList< SceneObject* > ObjectList;

class FiberNavigatorPluginInterface : public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
	Q_PLUGIN_METADATA(IID "Ibis.FiberNavigatorPluginInterface" )

public:

    FiberNavigatorPluginInterface();
    ~FiberNavigatorPluginInterface();
    QString OpenMaximaFile(QString fileName);
    QString OpenFAFile(QString fileName);
    virtual QString GetPluginName() override { return QString("FiberNavigator"); }
    virtual QString GetMenuEntryString() override { return QString("Fiber Navigator"); }
    virtual bool CanRun() override { return true; }
    virtual QWidget* CreateTab() override;
    virtual bool WidgetAboutToClose() override;

    //Parameters that can be changed from the interface
    void SetSeedPerAxis( int nb );
    int GetSeedPerAxis() { return m_seedPerAxis; }
    void GeneratePointInBox(int nb);
    void SetStepSize( double nb );
    double GetStepSize() { return m_stepSize; }
    void SetMinimumLength( int nb );
    int GetMinimumLength() { return m_minimumLength; }
    void SetMaximumLength( int nb );
    int GetMaximumLength() { return m_maximumLength; }
    void SetMaximumAngle( int nb );
    int GetMaximumAngle() { return m_maximumAngle; }
    void SetFAThreshold( double nb );
    double GetFAThreshold() { return m_FAThreshold; }
    void SetInertia( double nb );
    double GetInertia() { return m_inertia; }


    void SetReferenceId( int nb );
    int GetReferenceId() { return m_referenceId; }
    std::vector< std::pair< int, QString> > GetObjectsName();

    void SetFAMapLoad (bool use );
    bool isFAMapLoad() { return m_FAMapLoad; }
    void SetUseTubes( bool use );
    bool IsUsingTubes() { return m_useTubes; }
    bool IsRoiVisible() { return m_roiVisible; }
    void SetRoiVisibility( bool visible );

    void UpdateFibers();

    double* GetRoiSpacing();
    double* GetRoiCenter();
    double* GetRoiBounds();
    void SetRoiBounds(double xCenter, double yCenter, double zCenter,
                      double xSpacing, double ySpacing, double zSpacing);

signals:

    void RoiModifiedSignal();

public slots:

    void RoiModifiedSlot();
    virtual void SceneFinishedLoading();

protected:

    void CreateObject();
    void EnableRoi( bool on );

    //Complete Streamline pipeline
    void UpdateFibers( PolyDataObject * obj );
    vtkSmartPointer<vtkTransform> prepareTransformation();
    double* GetVolumeCenter();
    int GenerateStreamLine(vtkPoints* currentStreamLine, double* voxel, int orientation);
    bool IsInValidVoxel(double* voxel);
    bool TakeNextStep (Vec3 &previousDir, std::vector<float> &currentMaxima, Vec3 &currentPoint,
                       Vec3 &nextPoint, int orientation, int numberOfPointsOnFiber);
    void AddCurrentStreamLine(vtkCellArray* lines, vtkPoints* tractPts,vtkUnsignedCharArray* colorsArray,
                             vtkPoints* currentStreamLine, int numberOfPointsOnFiber, int &index);
    vtkSmartPointer<vtkPolyData> SetObjPolydata(PolyDataObject* obj, vtkPolyData* fiber);

    //Utilities
    double computeAngleBetweenSegment(Vec3 &PP, Vec3 &CP);
    double computeDotProduct(Vec3 &FP, Vec3 &LP);
    double computeSegmentLength(Vec3 &FP);
    double computeNorm(double vector[3]);

    int m_fiberObjectId;
    int m_referenceId;
    FiberObjectSettingsWidget* m_settingsWidget;

    Maximas m_maximas;
    fractionalAnisotropy m_FA;
    int* m_volumeShape;
    int m_seedPerAxis;
    std::vector< std::vector<int> > m_pointCloud;
    double m_stepSize;
    int m_minimumLength;
    int m_maximumLength;
    int m_maximumAngle;
    double m_FAThreshold;
    double m_inertia;

    bool m_useTubes;
    bool m_FAMapLoad;
    vtkSmartPointer<vtkTubeFilter> m_tubeFilter;
    vtkSmartPointer<vtkActor> m_actor;

    // ROI box
    bool m_roiVisible;
    bool m_firstVisible;
    vtkSmartPointer<vtkBoxWidget2> m_roi;
    vtkSmartPointer<vtkEventQtSlotConnect> m_roiCallbacks;
    vtkSmartPointer<vtkLinearTransform> m_previousInverse;

};


#endif
