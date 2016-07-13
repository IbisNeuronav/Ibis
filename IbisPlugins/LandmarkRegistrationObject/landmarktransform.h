/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef LANDMARKTRANSFORM_H
#define LANDMARKTRANSFORM_H

#include <QString>
#include "vtkObject.h"
#include <vector>
#include <sstream>

class vtkPoints;
class vtkIntArray;
class vtkDoubleArray;
class vtkLandmarkTransform;
class vtkLinearTransform;

#define INVALID_NUMBER 9999

class LandmarkTransform : public vtkObject
{
public:
    static LandmarkTransform * New() { return new LandmarkTransform; }
    vtkTypeMacro(LandmarkTransform,vtkObject);


    LandmarkTransform();
    virtual ~LandmarkTransform();

    void SetSourcePoints(vtkPoints *pts);
    void SetTargetPoints(vtkPoints *pts);

    double GetFRE(int index);
    double GetRMS(int index);
    double GetFinalRMS() {return this->FinalRMS;}

    bool IsScalingAllowed();
    void SetScalingAllowed( bool allow );

    vtkGetObjectMacro( RegistrationTransform, vtkLandmarkTransform );

    // Description:
    // recompute transformation based on current source and
    // target points.
    bool UpdateRegistrationTransform();
    bool InternalUpdate();
    void UpdateFRE();

    void Reset();

private:

    void Init();
    void Clear();

    vtkPoints * SourcePoints;
    vtkPoints * TargetPoints;
    vtkPoints * FiducialRegistrationError;
    vtkDoubleArray * FiducialRegistrationErrorMagnitude;
    vtkDoubleArray * FiducialRegistrationErrorRMS;
    double FinalRMS;
    vtkLandmarkTransform * InternalTransform;
    vtkLandmarkTransform * RegistrationTransform;

};

#endif // LANDMARKTRANSFORM_H
