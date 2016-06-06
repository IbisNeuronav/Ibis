/*=========================================================================

  Program:   Visualization Toolkit Bic Extension
  Module:    $RCSfile: vtkSimpleMINCReader.cxx,v $
  Language:  C++
  Date:      $Date: 2008-06-12 15:19:34 $
  Version:   $Revision: 1.4 $

  Copyright (c) 2002-2008 IPL, BIC, MNI, McGill, Simon Drouin, Eric Benoit
  All rights reserved.
  
=========================================================================*/
#include "vtkSimpleMINCReader.h"

vtkCxxRevisionMacro(vtkSimpleMINCReader, "$Revision: 1.4 $");


vtkSimpleMINCReader * vtkSimpleMINCReader::New()
{
    return new vtkSimpleMINCReader();
}


vtkSimpleMINCReader::vtkSimpleMINCReader()
{
    this->Icv = vtkMINCIcv::New();
    this->Icv->SetDataType( VTK_UNSIGNED_CHAR );
    this->Icv->SetXDimDir( MI_ICV_POSITIVE );
    this->Icv->SetYDimDir( MI_ICV_POSITIVE );
    this->Icv->SetZDimDir( MI_ICV_POSITIVE );
    this->Icv->DoRangeOn();
    this->Icv->DoNormOn();
}


vtkSimpleMINCReader::~vtkSimpleMINCReader()
{
}


void vtkSimpleMINCReader::SetOutputDataType( int type )
{
    this->Icv->SetDataType( type );
}


void vtkSimpleMINCReader::SetValidRange( double min, double max )
{
    this->Icv->SetValidMin( min );
    this->Icv->SetValidMax( max );
}


void vtkSimpleMINCReader::SetImageRange( double min, double max )
{
    this->Icv->UserNormOn();
    this->Icv->SetImageMin( min );
    this->Icv->SetImageMax( max );
}


void vtkSimpleMINCReader::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);
}
