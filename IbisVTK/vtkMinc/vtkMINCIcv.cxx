/*=========================================================================

  Program:   Visualization Toolkit Bic Extension
  Module:    $RCSfile: vtkMINCIcv.cxx,v $
  Language:  C++
  Date:      $Date: 2009-02-18 17:30:58 $
  Version:   $Revision: 1.5 $

  Copyright (c) 2007-2008 IPL, BIC, MNI, McGill, Simon Drouin
  All rights reserved.
  
=========================================================================*/
#include "vtkMINCIcv.h"


vtkCxxRevisionMacro(vtkMINCIcv, "$Revision: 1.5 $");


vtkMINCIcv * vtkMINCIcv::New()
{
    return new vtkMINCIcv();
}


void vtkMINCIcv::PrintSelf(ostream &os, vtkIndent indent)
{
}


vtkMINCIcv::vtkMINCIcv()
{
    this->IcvId = MI_ERROR;
    this->DataType = VTK_SHORT;
    this->DoRange = 1; 
    this->ValidMaxSet = 0;
    this->ValidMax = 1.0;
    this->ValidMinSet = 0;
    this->ValidMin = 0.0;
    this->DoNorm   = 0;
    this->UserNorm = 0;
    this->ImageMax = 1.0;
    this->ImageMin = 0.0;
    this->DoFillValue = 0;
    this->FillValue = VTK_DOUBLE_MIN;
    this->DoDimConv = 0;
    this->XDimDir = MI_ICV_POSITIVE;
    this->YDimDir = MI_ICV_POSITIVE;
    this->XDimDir = MI_ICV_POSITIVE;
}


vtkMINCIcv::~vtkMINCIcv()
{
    if( this->IcvId != MI_ERROR )
        miicv_free( this->IcvId );
}

void vtkMINCIcv::SetValidMax( double value )
{
    this->ValidMaxSet = 1;
    this->ValidMax = value;
}
    
void vtkMINCIcv::SetValidMin( double value )
{
    this->ValidMinSet = 1;
    this->ValidMin = value;
}

void vtkMINCIcv::SetupIcv()
{
    // free old icv
    if( this->IcvId != MI_ERROR )
    {
        miicv_free(this->IcvId);
        this->IcvId = MI_ERROR;
    }

    // create new icv
    this->IcvId = miicv_create();
    if ( this->IcvId == MI_ERROR )
    {
        vtkErrorMacro(<<"The minc icv could not be created.");
        return;
    }

    // Set the icv values
    nc_type mincDataType = NC_BYTE;
    int isSigned = 0;
    VtkDataTypeToMincDataType( this->DataType, mincDataType, isSigned );
    if( mincDataType == NC_NAT )
    {
        vtkErrorMacro(<<"The minc icv specifies an invalid minc data type.");
        return;
    }
    miicv_setint( this->IcvId, MI_ICV_TYPE, mincDataType );
    if( isSigned )
        miicv_setstr( this->IcvId, MI_ICV_SIGN, (char*)MI_SIGNED );
    else
        miicv_setstr( this->IcvId, MI_ICV_SIGN, (char*)MI_UNSIGNED );

    miicv_setint( this->IcvId, MI_ICV_DO_RANGE, this->DoRange );
    if( this->ValidMaxSet )
        miicv_setdbl( this->IcvId, MI_ICV_VALID_MAX, this->ValidMax );
    if( this->ValidMinSet )
        miicv_setdbl( this->IcvId, MI_ICV_VALID_MIN, this->ValidMin );
    miicv_setint( this->IcvId, MI_ICV_DO_NORM, this->DoNorm );
    miicv_setint( this->IcvId, MI_ICV_USER_NORM, this->UserNorm );
    miicv_setdbl( this->IcvId, MI_ICV_IMAGE_MAX, this->ImageMax );
    miicv_setdbl( this->IcvId, MI_ICV_IMAGE_MIN, this->ImageMin );
    miicv_setint( this->IcvId, MI_ICV_DO_FILLVALUE, this->DoFillValue );
    miicv_setdbl( this->IcvId, MI_ICV_FILLVALUE, this->FillValue );
    miicv_setint( this->IcvId, MI_ICV_DO_DIM_CONV, this->DoDimConv );
    miicv_setint( this->IcvId, MI_ICV_XDIM_DIR, this->XDimDir );
    miicv_setint( this->IcvId, MI_ICV_YDIM_DIR, this->YDimDir );
    miicv_setint( this->IcvId, MI_ICV_ZDIM_DIR, this->ZDimDir );
}


void vtkMINCIcv::VtkDataTypeToMincDataType( int vtkType, nc_type & mincType, int & isSigned )
{
    switch( vtkType )
        {
    case VTK_CHAR:
        isSigned = 1;
        mincType = NC_BYTE;
        break;
    case VTK_UNSIGNED_CHAR:
        isSigned = 0;
        mincType = NC_BYTE;
        break;
    case VTK_SHORT:
        isSigned = 1;
        mincType = NC_SHORT;
        break;
    case VTK_UNSIGNED_SHORT:
        isSigned = 0;
        mincType = NC_SHORT;
        break;
    case VTK_INT:
        isSigned = 1;
        mincType = NC_INT;
        break;
    case VTK_UNSIGNED_INT:
        isSigned = 0;
        mincType = NC_INT;
        break;
    case VTK_LONG:
        isSigned = 1;
        mincType = NC_INT;
        break;
    case VTK_UNSIGNED_LONG:
        isSigned = 0;
        mincType = NC_INT;
        break;
    case VTK_FLOAT:
        isSigned = 1;
        mincType = NC_FLOAT;
        break;
    case VTK_DOUBLE:
        isSigned = 1;
        mincType = NC_DOUBLE;
        break;
    default:
        isSigned = 1;
        mincType = NC_NAT;
        break;
    }
}


// This function doesn't produce error on invalid input types. The client
// must pop the error when VTK_VOID is returned
int vtkMINCIcv::MincDataTypeToVtkDataType( nc_type mincType, int isSigned )
{
    int ret = VTK_VOID;
    switch( mincType )
    {
    case NC_BYTE:
        ret = isSigned ? VTK_CHAR : VTK_UNSIGNED_CHAR;
        break;
    case NC_SHORT:
        ret = isSigned ? VTK_SHORT : VTK_UNSIGNED_SHORT;
        break;
    case NC_INT:
        ret = isSigned ? VTK_INT : VTK_UNSIGNED_INT;
        break;
    case NC_FLOAT:
        ret = VTK_FLOAT;
        break;
    case NC_DOUBLE:
        ret = VTK_DOUBLE;
        break;
    case NC_NAT:
    case NC_CHAR:
    default:
        break;
    }

    return ret;
}

