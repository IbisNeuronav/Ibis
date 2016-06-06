/*=========================================================================

  Program:   Visualization Toolkit Bic Extension
  Module:    $RCSfile: vtkMINCIcv.h,v $
  Language:  C++
  Date:      $Date: 2008-06-12 15:19:34 $
  Version:   $Revision: 1.4 $

  Copyright (c) 2002-2008 IPL, BIC, MNI, McGill, Simon Drouin
  All rights reserved.
  
=========================================================================*/
// .NAME vtkMINCIcv - Specify loading parameter for minc files
// .SECTION Description
// Let the user specify the format of the output of vtkMINCReader. If an
// instance of this class is not provided to vtkMINCReader, then the data
// is loaded to the format found in the file. All non-readonly attributes 
// of real minc icv can be set through this class and have corresponding 
// names, except dataType that has been changed to correspond to values
// supported by vtk. The names of the real minc icv properties have been
// writen has commentary above the declaration of the variable and accessor
// function.
// .SECTION See Also
// vtkMINCReader

#ifndef VTKMINCICV_H
#define VTKMINCICV_H


extern "C"
{
#include "minc.h"
}
#include "vtkObject.h"


#define MAX_NUMBER_OF_DIMENSIONS 4            // Number of dimensions with and


class vtkMINCIcv : public vtkObject
{

public:

    static vtkMINCIcv * New();
    vtkTypeRevisionMacro(vtkMINCIcv,vtkObject);

    void PrintSelf(ostream &os, vtkIndent indent);

    vtkGetMacro(IcvId,int);
    
    // Specifies the type of values that the user wants. Modifying this property automatically causes MI_ICV_VALID_MAX and MI_ICV_VALID_MIN to be set to their default values. Default = VTK_UNSIGNED_CHAR.
    vtkSetMacro(DataType,int);
    vtkGetMacro(DataType,int);
    
    // When TRUE, range conversions (for valid max and min and for value normalization) are done. When FALSE, values are not modified. Default = TRUE.
    vtkSetMacro(DoRange,int);
    vtkBooleanMacro(DoRange,int);
    
    // MI_ICV_VALID_MAX (type double) : Valid maximum value (ignored for floating point types). Default = maximum legal value for type and sign (1.0 for floating point types).
    void SetValidMax( double value );
    
    // MI_ICV_VALID_MIN (type double) : Valid minimum value (ignored for floating point types). Default = minimum legal value for type and sign (1.0 for floating point types).
    void SetValidMin( double value );
    
    // MI_ICV_DO_NORM (type int) : If TRUE, then normalization of values is done (see user guide for details of normalization). Default = FALSE.
    vtkSetMacro(DoNorm,int);
    vtkBooleanMacro(DoNorm,int);
    
    // MI_ICV_USER_NORM (type int) : If TRUE, then the user specifies the normalization maximum and minimum. If FALSE, values are taken as maximum and minimum for whole image variable. Default = FALSE.
    vtkSetMacro(UserNorm,int);
    vtkBooleanMacro(UserNorm,int);
    
    // MI_ICV_IMAGE_MAX (type double) : Image maximum for user normalization. Default = 1.0.
    vtkSetMacro(ImageMax,double);
    
    // MI_ICV_IMAGE_MIN (type double) : Image minimum for user normalization. Default = 0.0.
    vtkSetMacro(ImageMin,double);
    
    // MI_ICV_DO_FILLVALUE (type int) : If set to TRUE, then range checking is done on input and values that are out of range (value in the file less than MIvalid_min or greater than MIvalid_max) are set to the value of MI_ICV_FILLVALUE. Default = FALSE.
    vtkSetMacro(DoFillValue,int);
    vtkBooleanMacro(DoFillValue,int);
    
    // MI_ICV_FILLVALUE (type double) : Value to use when pixels are out of range (on input only). This value is written to user's buffer directly without any scaling. Default = -DBL_MAX.
    vtkSetMacro(FillValue,double);
    
    // MI_ICV_DO_DIM_CONV (type int) : If set to TRUE, then dimension conversions may be done. Default = FALSE.
    vtkSetMacro(DoDimConv,int);
    
    // Indicates the desired orientation of the x image dimensions (MIxspace and MIxfrequency). Values can be one of MI_ICV_POSITIVE, MI_ICV_NEGATIVE or MI_ICV_ANYDIR. A positive orientation means that the MIstep attribute of the dimension is positive. MI_ICV_ANYDIR means that no flipping is done. Default = MI_ICV_POSITIVE.
    vtkSetMacro(XDimDir,int); 
    
    // Indicates the desired orientation of the y image dimensions. Default = MI_ICV_POSITIVE.    
    vtkSetMacro(YDimDir,int);
    
    // Indicates the desired orientation of the z image dimensions. Default = MI_ICV_POSITIVE
    vtkSetMacro(ZDimDir,int);
    
    void SetupIcv();
    
protected:

    vtkMINCIcv();
    ~vtkMINCIcv();

    static void VtkDataTypeToMincDataType( int vtkDataType, nc_type & mincType, int & isSigned );
    static int  MincDataTypeToVtkDataType( nc_type mincType, int isSigned );

    // The id of the minc icv created
    int IcvId;

    // Specifies the type of values that the user wants. Modifying this property automatically causes MI_ICV_VALID_MAX and MI_ICV_VALID_MIN to be set to their default values. Default = VTK_UNSIGNED_CHAR.
    int DataType;
    // When TRUE, range conversions (for valid max and min and for value normalization) are done. When FALSE, values are not modified. Default = TRUE.
    int DoRange;
    // MI_ICV_VALID_MAX (type double) : Valid maximum value (ignored for floating point types). Default = maximum legal value for type and sign (1.0 for floating point types).
    int ValidMaxSet;
    double ValidMax;
    // MI_ICV_VALID_MIN (type double) : Valid minimum value (ignored for floating point types). Default = minimum legal value for type and sign (1.0 for floating point types).
    int ValidMinSet;
    double ValidMin;
    // MI_ICV_DO_NORM (type int) : If TRUE, then normalization of values is done (see user guide for details of normalization). Default = FALSE.
    int DoNorm;
    // MI_ICV_USER_NORM (type int) : If TRUE, then the user specifies the normalization maximum and minimum. If FALSE, values are taken as maximum and minimum for whole image variable. Default = FALSE.
    int UserNorm;
    // MI_ICV_IMAGE_MAX (type double) : Image maximum for user normalization. Default = 1.0.
    double ImageMax;
    // MI_ICV_IMAGE_MIN (type double) : Image minimum for user normalization. Default = 0.0.
    double ImageMin;
    // MI_ICV_DO_FILLVALUE (type int) : If set to TRUE, then range checking is done on input and values that are out of range (value in the file less than MIvalid_min or greater than MIvalid_max) are set to the value of MI_ICV_FILLVALUE. Default = FALSE.
    int DoFillValue;
    // MI_ICV_FILLVALUE (type double) : Value to use when pixels are out of range (on input only). This value is written to user's buffer directly without any scaling. Default = -DBL_MAX.
    double FillValue;
    // MI_ICV_DO_DIM_CONV (type int) : If set to TRUE, then dimension conversions may be done. Default = FALSE.
    int DoDimConv;
    // Indicates the desired orientation of the x image dimensions (MIxspace and MIxfrequency). Values can be one of MI_ICV_POSITIVE, MI_ICV_NEGATIVE or MI_ICV_ANYDIR. A positive orientation means that the MIstep attribute of the dimension is positive. MI_ICV_ANYDIR means that no flipping is done. Default = MI_ICV_POSITIVE.
    int XDimDir ;
    // Indicates the desired orientation of the y image dimensions. Default = MI_ICV_POSITIVE.    
    int YDimDir;
    // Indicates the desired orientation of the z image dimensions. Default = MI_ICV_POSITIVE
    int ZDimDir; 

    friend class vtkMINCReader;

private:
    
    vtkMINCIcv(const vtkMINCIcv&);      // Not implemented.
    void operator=(const vtkMINCIcv&);  // Not implemented.
    
};
#endif




