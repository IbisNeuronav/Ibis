/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __UsProbeObject_h_
#define __UsProbeObject_h_

#include "sceneobject.h"
#include <map>
#include "hardwaremodule.h"

class vtkImageData;
class vtkImageActor;
class vtkImageProperty;
class vtkImageMapToColors;
class vtkImageToImageStencil;
class vtkImageStencil;
class USMask;

class UsProbeObject : public SceneObject
{

Q_OBJECT
    
public:
        
    static UsProbeObject * New() { return new UsProbeObject; }
    vtkTypeMacro( UsProbeObject, SceneObject );
    
    UsProbeObject();
    virtual ~UsProbeObject();

    void AddClient();
    void RemoveClient();
    
    void SetUseMask( bool useMask );
    bool GetUseMask() { return m_maskOn; }
    USMask *GetMask() { return m_mask; }

    // Implementation of standard SceneObject method
    virtual bool Setup( View * view );
    virtual bool Release( View * view );
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets);


    TrackerToolState GetState();
    int GetVideoImageWidth();
    int GetVideoImageHeight();
    int GetVideoImageNumberOfComponents();
    vtkImageData * GetVideoOutput();
    vtkTransform * GetTransform();
    vtkTransform * GetUncalibratedTransform();

    int GetNumberOfCalibrationMatrices();
    QString GetCalibrationMatrixName( int index );
    void SetCurrentCalibrationMatrixName( QString name );
    QString GetCurrentCalibrationMatrixName();
    void SetCurrentCalibrationMatrix( vtkMatrix4x4 * mat );
    vtkMatrix4x4 * GetCurrentCalibrationMatrix();

    enum ACQ_TYPE {ACQ_B_MODE = 0, ACQ_DOPPLER = 1, ACQ_POWER_DOPPLER = 2};
    void SetAcquisitionType(ACQ_TYPE type);
    ACQ_TYPE GetAcquisitionType() { return m_acquisitionType; }    

    void InitialSetMask( USMask *mask );

    int GetNumberOfAvailableLUT();
    QString GetLUTName( int index );
    void SetCurrentLUTIndex( int index );
    int GetCurrentLUTIndex() { return m_lutIndex; }

private slots:

    void OnUpdate();
    void UpdateMask();

protected:

    void ObjectAddedToScene();
    void ObjectRemovedFromScene();
    virtual void Hide();
    virtual void Show();

    bool m_maskOn;
    int m_lutIndex;
    USMask                 * m_mask;
    USMask                 * m_defaultMask;
    vtkImageProperty       * m_sliceProperties;
    vtkImageMapToColors    * m_mapToColors;
    vtkImageToImageStencil * m_imageStencilSource;
    vtkImageStencil        * m_sliceStencil;
    vtkTransform           * m_toolTransform;

    struct PerViewElements
    {
        PerViewElements() : imageActor(0) {}
        vtkImageActor * imageActor;
    };
    typedef std::map<View*,PerViewElements> PerViewContainer;
    PerViewContainer m_perViews;

    void UpdatePipeline();
    void UpdatePipelineOneView( PerViewElements & pv );

    ACQ_TYPE m_acquisitionType;

};

#endif
