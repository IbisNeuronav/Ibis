/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_GENERATEDSURFACE_H_
#define TAG_GENERATEDSURFACE_H_

#include <QVector>
#include <qstring.h>
#include "serializer.h"
#include "polydataobject.h"

class vtkPolyData;
class ImageObject;
class vtkImageData;
class vtkScalarsToColors;
class vtkImageAccumulate;
class SurfaceSettingsWidget;
class ContourSurfacePluginInterface;



#define DEFAULT_RADIUS 1.0
#define DEFAULT_STANDARD_DEVIATION 1.0

class GeneratedSurface : public PolyDataObject
{
public:
    
    static GeneratedSurface *New() { return new GeneratedSurface; }
    vtkTypeMacro(GeneratedSurface,PolyDataObject);

    virtual void Serialize( Serializer * ser );

    virtual vtkPolyData * GenerateSurface();

    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets);
    SurfaceSettingsWidget * CreateSurfaceSettingsWidget(QWidget * parent);
    void UpdateSettingsWidget();
    void SetPluginInterface( ContourSurfacePluginInterface * interf );
    bool IsValid() { return m_imageObject != 0; }

    void SetImageObject(ImageObject *obj);
    double GetContourValue() {return m_contourValue;}
    void SetContourValue(double cv){m_contourValue = cv;}
    int GetReduction() {return m_reductionPercent;}
    void SetReduction(int val){m_reductionPercent = val;}
    vtkImageAccumulate *GetImageHistogram();
    vtkScalarsToColors * GetImageLut();
    void SetImageLutRange(double range[2]);

    void GetImageScalarRange(double range[2]);

    //Gaussian smoothing of the generated surface
    bool GetGaussianSmoothingFlag() {return m_gaussianSmoothing;}
    double GetRadiusFactor() {return m_radius;}
    double GetStandardDeviation() {return m_standardDeviation;}
    void SetGaussianSmoothingFlag(bool flag) {m_gaussianSmoothing = flag;}
    void SetRadiusFactor(double radiusFactor){m_radius = radiusFactor;}
    void SetStandardDeviation(double deviation){m_standardDeviation = deviation;}

protected:
    ContourSurfacePluginInterface * m_pluginInterface;
    ImageObject *m_imageObject;
    double m_contourValue;

    //Gaussian smoothing of the surface
    bool m_gaussianSmoothing;
    double m_radius;
    double m_standardDeviation;
    int m_reductionPercent;
    
    GeneratedSurface();
    virtual ~GeneratedSurface();
    
};
    
ObjectSerializationHeaderMacro( GeneratedSurface );

#endif //TAG_GENERATEDSURFACE_H_
