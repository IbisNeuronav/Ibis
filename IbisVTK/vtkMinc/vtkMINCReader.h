/*=========================================================================

  Program:   Visualization Toolkit Bic Extension
  Module:    $RCSfile: vtkMINCReader.h,v $
  Language:  C++
  Date:      $Date: 2010-04-08 14:45:57 $
  Version:   $Revision: 1.8 $

  Copyright (c) 2002-2008 IPL, BIC, MNI, McGill, Simon Drouin, Eric Benoit 
  All rights reserved.

=========================================================================*/
// .NAME vtkMINCReader - read MINC files
// .SECTION Description

// .SECTION See Also
// vtkMINCIcv vtkImageDimensionReorder vtkMINCWriter

#ifndef VTKMINCREADER_H
#define VTKMINCREADER_H

#include <vtkImageReader2.h>
extern "C"
{
#include "minc.h"
}
#include "vtkMINCIcv.h"

class vtkTransform;
class vtkMatrix4x4;
class vtkMINCImageAttributes2;

enum READERSTATUS { MINC_OK, MINC_CORRUPTED, MINC_NEGATIVE_STEP};


class vtkMINCReader : public vtkImageReader2
{

public:

    static vtkMINCReader * New();
    vtkTypeRevisionMacro(vtkMINCReader,vtkImageReader2);

    void PrintSelf(ostream &os, vtkIndent indent);

    // Description: is the given file name a MINC file?
    virtual int CanReadFile(const char* fname);

    // Description:
    // Get the file extensions for this format.
    // Returns a string with a space separated list of extensions in 
    // the format .extension
    virtual const char* GetFileExensions()
    {
        return ".mnc .minc";
    }

    // Description: 
    // Return a descriptive name for the file format that might be useful in a GUI.
    virtual const char* GetDescriptiveName()
    {
        return "MINC";
    }

    void SetIcv( vtkMINCIcv * icv );
    
    vtkGetObjectMacro(Transform,vtkTransform);
    vtkSetMacro(UseTransformForStartAndStep,int);

    // Description:
    // Get a matrix that describes the orientation of the data.
    // The three columns of the matrix are the direction cosines
    // for the x, y and z dimensions respectively.
    vtkMatrix4x4 * GetDirectionCosines() {return DirectionCosinesMatrix;}

    vtkGetMacro(ReaderStatus,int);
    vtkGetMacro(ImageDataType,int);

    virtual vtkMINCImageAttributes2 *GetImageAttributes();
    virtual const char* GetComment()
    {
        return &Comment[0];
    }
    virtual double GetTimeStamp()
    {
        return TimeStamp;
    }

protected:

    vtkMINCReader();
    ~vtkMINCReader();

    virtual void ExecuteInformation();
    virtual void ExecuteData(vtkDataObject *out);

    vtkMINCIcv * Icv;

    vtkTransform * Transform;
    vtkMatrix4x4 * DirectionCosinesMatrix;
    
    // User specified attributes
    int UseTransformForStartAndStep;

    // Attributes directly read from the input file
    int     MincFileId;
    int     ImageVarId;
    int     ImageDataType;
    int     DimIndex[4];
    long    DimensionLength[ 4 ];
    char    DimensionNames[ 4 ][ MAX_NC_NAME ];
    double  Step[4];
    double  Origin[4];
    double  DirectionCosines[4][3];

    // Computed attributes
    int  DimensionOrder[3];
    long NumberOfScalarComponents;
    int  NumberOfSpacialDimensions;
    
    int ReaderStatus;
    char Comment[1024];
    double TimeStamp;

    virtual int ReadMINCFileAttributes();
    virtual int IndexFromDimensionName(const char *dimName);
    vtkMINCImageAttributes2 *ImageAttributes;

private:

    vtkMINCReader(const vtkMINCReader&);  // Not implemented.
    void operator=(const vtkMINCReader&);  // Not implemented.
};

#endif



