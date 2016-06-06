/*=========================================================================

  Program:   Visualization Toolkit Bic Extension
  Module:    $RCSfile: vtkMINCReader.cxx,v $
  Language:  C++
  Date:      $Date: 2010-04-08 14:45:57 $
  Version:   $Revision: 1.13 $

  Copyright (c) 2002-2008 IPL, BIC, MNI, McGill, Simon Drouin, Eric Benoit
  All rights reserved.

=========================================================================*/
#include "vtkMINCReader.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkMINCImageAttributes2.h"
#include "vtkStringArray.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkMINC2.h"

#define VTK_MINC_MAX_DIMS 8

vtkCxxRevisionMacro(vtkMINCReader, "$Revision: 1.13 $");


vtkMINCReader * vtkMINCReader::New()
{
    return new vtkMINCReader();
}


void vtkMINCReader::SetIcv( vtkMINCIcv * icv )
{
    if( this->Icv != NULL )
        this->Icv->Delete();

    this->Icv = icv;
    this->Icv->Register( this );
}


//-------------------------------------------------------------------------------
vtkMINCReader::vtkMINCReader()
{
    this->Icv = NULL;
    this->Transform = vtkTransform::New();
    this->Transform->Identity();
    this->DirectionCosinesMatrix = vtkMatrix4x4::New();
    this->DirectionCosinesMatrix->Identity();
    
    this->MincFileId = -1;
    this->ImageVarId = -1;
    this->ImageDataType = VTK_UNSIGNED_CHAR;
    this->NumberOfScalarComponents = 1;
    for( int i = 0; i < 4; i++ )
    {
        this->DimIndex[i] = -1;
        this->DimensionLength[i] = 1;
        this->DimensionNames[1][0] = '\0';
        this->Step[i] = 1;
        this->Origin[i] = 0;
        for( int j = 0; j < 3; j++ )
        {
            if( i == j )
                this->DirectionCosines[i][j] = 1.0;
            else
                this->DirectionCosines[i][j] = 0.0;
        }
    }

    this->DimensionOrder[0] = 0;
    this->DimensionOrder[1] = 1;
    this->DimensionOrder[2] = 2;
    ncopts = NC_VERBOSE; // | NC_FATAL;
    this->UseTransformForStartAndStep = 0;
    this->ReaderStatus = MINC_OK;
    this->ImageAttributes = vtkMINCImageAttributes2::New();
    this->Comment[0] = 0;
    this->TimeStamp = 0.0;
}


//-------------------------------------------------------------------------------
vtkMINCReader::~vtkMINCReader()
{
    if( this->Icv != NULL )
    {
        this->Icv->Delete();
    }
    if( this->MincFileId != MI_ERROR )
        miclose( this->MincFileId );

    this->Transform->Delete();
    this->DirectionCosinesMatrix->Delete();
    if (this->ImageAttributes)
        this->ImageAttributes->Delete();
}

//-------------------------------------------------------------------------------
int vtkMINCReader::CanReadFile( const char * fname )
{
    // First do a very rapid check of the magic number (from David Gobbi's Minc reader)
    FILE *fp = fopen(fname, "rb");
    if (!fp)
    {
        return 0;
    }

    char magic[4];
    fread(magic, 4, 1, fp);
    fclose(fp);

    if (magic[0] != 'C' ||
        magic[1] != 'D' ||
        magic[2] != 'F' ||
        magic[3] != '\001')
    {
        return 0;
    }

    // now Simon's code
    char name[1024];       // filenames shouldn't be longer than that
    strcpy( name, fname ); // have to do that because miopen doesn't take const char *
    this->MincFileId = miopen( name, NC_NOWRITE );
    if ( this->MincFileId == -1 )
    {
        return 0;
    }
    miclose( this->MincFileId );
    this->MincFileId = MI_ERROR;
    return 3;
}

//----------------------------------------------------------------------------
// This templated function reorders pixels for any type of data.
template < class type >
void vtkReorderVoxels(   vtkMINCReader * self,
                         type * inptr,
                         type * outptr,
                         int outputStep[3],
                         int outputExtent[6],
                         int numberOfScalarComponent )
{
    type * inIt[ 3 ];
    inIt[ 0 ] = inptr;
    inIt[ 1 ] = inptr;
    inIt[ 2 ] = inptr;
    type * outIt = outptr;
    int numberOfSlices = outputExtent[5] - outputExtent[4] + 1;
    double percent = 0;

    self->SetProgressText("Reordering voxels");

    for( int i = outputExtent[ 4 ] ; i <= outputExtent[ 5 ]; i++ )
    {
        inIt[ 1 ] = inIt[ 2 ];
        for( int j = outputExtent[ 2 ]; j <= outputExtent[ 3 ] ; j++ )
        {
            inIt[ 0 ] = inIt[ 1 ];
            for( int k = outputExtent[ 0 ]; k <= outputExtent[ 1 ]; k++ )
            {
                for( int l = 0; l < numberOfScalarComponent; l++ )
                {
                    outIt[l] = inIt[0][l];
                }
                outIt += numberOfScalarComponent;
                inIt[ 0 ] += outputStep[ 0 ];
            }
            inIt[ 1 ] += outputStep[ 1 ];
        }
        inIt[ 2 ] += outputStep[ 2 ];
        percent = (double)( i - outputExtent[4] ) / numberOfSlices;
        self->UpdateProgress( percent );
    }
}


//-------------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkMINCReader::ExecuteData(vtkDataObject *output)
{
    if (this->ReaderStatus != MINC_OK)
        return;

    vtkImageData * imageData = this->AllocateOutputData(output);
    
    //=============================================
    // Open the minc file
    //=============================================
    if ( this->InternalFileName == NULL )
    {
        vtkErrorMacro(<< "Either a FileName or FilePrefix must be specified.");
        this->ReaderStatus = MINC_CORRUPTED;
        return;
    }

    if( this->MincFileId != MI_ERROR )
    {
        miclose( this->MincFileId );
    }

    this->MincFileId = miopen( this->InternalFileName, NC_NOWRITE );
    if ( this->MincFileId == MI_ERROR )
    {
        vtkErrorMacro(<<"Could not open minc file.");
        this->ReaderStatus = MINC_CORRUPTED;
        return;
    }

    this->ComputeDataIncrements();
    imageData->GetPointData()->GetScalars()->SetName("MINCImage");

    //=============================================
    // Compute the starts and lengths to read all
    // voxels.
    //=============================================
    int extent[6];
    imageData->GetExtent( extent );

    long start[4] = { 0, 0, 0, 0 };
    long dimLength[4] = { 1, 1, 1, 1 };

    this->SetProgressText("Reading voxels");

    int i;
    for( i = 0; i < 3; i++ )
    {
        if( this->DimIndex[i] != -1 )
        {
            start[ this->DimIndex[i] ] = extent[ 2 * i ];
            dimLength[ this->DimIndex[i] ] = extent[ 2 * i + 1 ] - extent[ 2 * i ] + 1;
        }
    }
    if( this->DimIndex[3] != -1 )
    {
        dimLength[ this->DimIndex[3] ] = this->NumberOfScalarComponents;
    }


    //=============================================
    // Read voxels
    //=============================================
    vtkImageData * tempImage = vtkImageData::New();
    tempImage->SetNumberOfScalarComponents( this->NumberOfScalarComponents );
    tempImage->SetScalarType( this->GetDataScalarType() );
    tempImage->SetExtent( extent );
    tempImage->AllocateScalars();
    void * outPtr = tempImage->GetScalarPointer();
    if( this->Icv )
    {
        int id = this->Icv->GetIcvId();
        if (miicv_get( id, start, dimLength, outPtr ) == MI_ERROR)
        {
            this->ReaderStatus = MINC_CORRUPTED;
            miclose( this->MincFileId );
            this->MincFileId = MI_ERROR;
            return;
        }
    }
    else
    {
        nc_type mincDataType = NC_SHORT;
        int isSigned = 1;
        char sign[9] = MI_SIGNED;
        vtkMINCIcv::VtkDataTypeToMincDataType( this->GetDataScalarType(), mincDataType, isSigned );
        strcpy( sign, isSigned ? MI_SIGNED : MI_UNSIGNED );
        if( mivarget( this->MincFileId,
                      this->ImageVarId,
                      start, dimLength,
                      mincDataType,
                      sign,
                      outPtr )  == MI_ERROR )
        {
            vtkErrorMacro(<<"Error getting image voxels." );
            this->ReaderStatus = MINC_CORRUPTED;
            miclose( this->MincFileId );
            this->MincFileId = MI_ERROR;
            return;
        }
    }


    //=============================================
    // Reorder voxels
    //=============================================
    int inputDim[3];
    int inputStep[3];
    inputStep[0] = this->NumberOfScalarComponents;
    inputStep[1] = this->NumberOfScalarComponents;
    inputStep[2] = this->NumberOfScalarComponents;



    for( i = 0; i < 3; i++ )
    {
        inputDim[ i ] = dimLength[ this->NumberOfSpacialDimensions - i - 1 ];
        for( int j = 0; j < i; j++ )
        {
            inputStep[ i ] *= inputDim[ j ];
        }
    }

    int outputStep[3];
    for( i = 0; i < 3; i++ )
    {
        outputStep[ this->DimensionOrder[ i ] ] = inputStep[ i ];
    }

    switch ( imageData->GetScalarType() )
    {
    vtkTemplateMacro( vtkReorderVoxels(
                      this,
                      static_cast< VTK_TT * >(tempImage->GetScalarPointer()),
                      static_cast< VTK_TT * >(imageData->GetScalarPointer()),
                      outputStep, extent, this->NumberOfScalarComponents ) );
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      this->ReaderStatus = MINC_CORRUPTED;
      miclose( this->MincFileId );
      this->MincFileId = MI_ERROR;
      return;
    }

    if( this->MincFileId != MI_ERROR )
    {
        miclose( this->MincFileId );
        this->MincFileId = MI_ERROR;
    }
    tempImage->Delete();
}


//------------------------------------------------------------------------------
void vtkMINCReader::ExecuteInformation()
{
    this->ComputeInternalFileName(this->DataExtent[4]);
    if ( this->InternalFileName == NULL )
    {
        vtkErrorMacro(<<"No valid minc file name.");
        this->ReaderStatus = MINC_CORRUPTED;
        return;
    }

    if( this->MincFileId != MI_ERROR )
    {
        miclose( this->MincFileId );
    }

    //=============================================
    // Open the minc file
    //=============================================
    this->MincFileId = miopen( this->InternalFileName, NC_NOWRITE );
    if ( this->MincFileId == -1 )
    {
        vtkErrorMacro(<<"Could not open minc file.");
        this->ReaderStatus = MINC_CORRUPTED;
        this->DataExtent[0] = 0;
        this->DataExtent[1] = 0;
        this->DataExtent[2] = 0;
        this->DataExtent[3] = 0;
        this->DataExtent[4] = 0;
        this->DataExtent[5] = 0;
        this->SetNumberOfScalarComponents(1);
        this->vtkImageReader2::ExecuteInformation();
        return;
    }

    //=============================================
    // Get the id of the image variable
    //=============================================
    if( nc_inq_varid ( this->MincFileId, MIimage, &( this->ImageVarId ) ) == MI_ERROR )
    {
        vtkErrorMacro(<<"Could not get the id of the image variable");
        this->ReaderStatus = MINC_CORRUPTED;
        miclose( this->MincFileId );
        this->MincFileId = MI_ERROR;
        return;
    }

    //=============================================
    // Create an icv and attach it to the image
    // variable.
    //=============================================
    if( this->Icv != NULL )
    {
        Icv->SetupIcv();
        miicv_attach( Icv->GetIcvId(), this->MincFileId, this->ImageVarId );
    }


    //=============================================
    // Get the number of dimensions
    //=============================================
    int numberOfDimensions;
    if( ncinquire( this->MincFileId, &numberOfDimensions, NULL, NULL, NULL) == MI_ERROR )
    {
        vtkErrorMacro(<<"Error getting the number of dimensions");
        this->ReaderStatus = MINC_CORRUPTED;
        return;
    }

    if( numberOfDimensions > MAX_NUMBER_OF_DIMENSIONS )
    {
        vtkErrorMacro(<<"The maximum number of dimensions supported by this reader is " << MAX_NUMBER_OF_DIMENSIONS << " (including vector dimension)");
        this->ReaderStatus = MINC_CORRUPTED;
        miclose( this->MincFileId );
        this->MincFileId = MI_ERROR;
        return;
    }

    //=============================================
    // Get the attributes of every dimension
    //=============================================
    int dimVarId = MI_ERROR;
    int i, j;
    for( i = 0 ; i < numberOfDimensions; i++ )
    {
        if( ncdiminq( this->MincFileId, i, this->DimensionNames[i], NULL ) == MI_ERROR )
        {
            vtkErrorMacro(<<"Error getting dimension " << i << " attributes." << endl);
            this->ReaderStatus = MINC_CORRUPTED;
            miclose( this->MincFileId );
            this->MincFileId = MI_ERROR;
            return;
        }
        else
        {
            if( !strcmp( MIxspace, this->DimensionNames[i] ) )
            {
                this->DimIndex[0] = i;
            }
            else if( !strcmp( MIyspace, this->DimensionNames[i] ) )
            {
                this->DimIndex[1] = i;
            }
            else if( !strcmp( MIzspace, this->DimensionNames[i] ) )
            {
                this->DimIndex[2] = i;
            }
            else if( !strcmp( MIvector_dimension, this->DimensionNames[i] ) )
            {
                this->DimIndex[3] = i;
            }
        }
        if( ncdiminq( this->MincFileId, i, NULL, &(this->DimensionLength[i]) ) == MI_ERROR )
        {
            vtkErrorMacro(<<"Error getting dimension " << i << " length.");
            this->ReaderStatus = MINC_CORRUPTED;
            miclose( this->MincFileId );
            this->MincFileId = MI_ERROR;
            return;
        }

        if( strcmp( MIvector_dimension, this->DimensionNames[i] )  !=0 )
        {
          dimVarId = ncvarid( this->MincFileId, this->DimensionNames[i] );

          if( dimVarId != MI_ERROR )
          {
              miattget1( this->MincFileId, dimVarId, (char*)MIstep, NC_DOUBLE, &(this->Step[i]) );
              miattget1( this->MincFileId, dimVarId, (char*)MIstart, NC_DOUBLE, &(this->Origin[i]) );
              miattget( this->MincFileId, dimVarId, (char*)MIdirection_cosines, NC_DOUBLE, 4, &(this->DirectionCosines[i]), NULL );
              if( this->Step[i] < 0 )
              {
                  vtkWarningMacro(<< " At least one of the step size is negative in the image. This is not well supported by some display classes. To changed that: mincreshape +direction <input_file> <output_file>" << endl );
                  this->ReaderStatus = MINC_NEGATIVE_STEP;
              }
          }
        }
    }

    this->NumberOfSpacialDimensions = numberOfDimensions;
    if( this->DimIndex[3] != -1 )
    {
        this->NumberOfSpacialDimensions--;
        this->NumberOfScalarComponents = this->DimensionLength[ this->DimIndex[3] ];
        if( this->DimIndex[3] != this->NumberOfSpacialDimensions )
        {
            vtkErrorMacro(<<"Error: this reader does not support minc files where vector dimension is not the last");
            this->ReaderStatus = MINC_CORRUPTED;
            miclose( this->MincFileId );
            this->MincFileId = MI_ERROR;
            return;
        }
    }

    //=============================================
    // compute the dimension order
    //=============================================
    for( i = 0; i < 3; i++ )
    {
        if( this->DimIndex[i] != -1 )
        {
            this->DimensionOrder[ this->NumberOfSpacialDimensions - this->DimIndex[i] - 1 ] = i;
        }
    }

    //=============================================
    // Set DirectionCosinesMatrix
    //=============================================
    for( i = 0; i < 3; i++ )
    {
        if( this->DimIndex[i] != -1 )
        {
            for( j = 0; j < 3; j++ )
            {
                this->DirectionCosinesMatrix->SetElement( i, j, this->DirectionCosines[this->DimIndex[i]][j] );
            }
        }
    }

    //=============================================
    // Compute a transform from start, step and
    // direction cosines.
    //=============================================
    double translation[3] = { 0.0, 0.0, 0.0 };
    double scale[3] = { 1.0, 1.0, 1.0 };
    vtkMatrix4x4 * rotation = vtkMatrix4x4::New();

    for( i = 0; i < 3; i++ )
    {
        if( this->DimIndex[i] != -1 )
        {
            translation[i] = this->Origin[ this->DimIndex[i] ];
            scale[i] = this->Step[ this->DimIndex[i] ];
            for( j = 0; j < 3; j++ )
            {
                rotation->SetElement( j, i, this->DirectionCosines[this->DimIndex[i]][j] );
            }
        }
    }

    this->Transform->Identity();
    this->Transform->Concatenate( rotation );
    if( this->UseTransformForStartAndStep )
    {
        this->Transform->Translate( translation );
        this->Transform->Scale( scale );
    }
    else
    {
        this->SetDataSpacing( scale[0], scale[1], scale[2] );
        this->SetDataOrigin( translation[0], translation[1], translation[2] );
    }
    rotation->Delete();

    //=============================================
    // Get the image variable data type
    //=============================================
    if( !this->Icv )
    {
        nc_type imageDataType;
        int imageDataTypeIsSigned;
        if( miget_datatype( this->MincFileId, this->ImageVarId, &imageDataType, &imageDataTypeIsSigned ) == MI_ERROR )
        {
            vtkErrorMacro(<<"Could not get image data type");
            this->ReaderStatus = MINC_CORRUPTED;
            miclose( this->MincFileId );
            this->MincFileId = MI_ERROR;
            return;
        }
        this->ImageDataType = vtkMINCIcv::MincDataTypeToVtkDataType( imageDataType, imageDataTypeIsSigned );
    }
    else
    {
        this->ImageDataType = Icv->GetDataType();
    }


    //=============================================
    // Set image output extent
    //=============================================
    this->DataExtent[0] = 0;
    this->DataExtent[1] = 0;
    this->DataExtent[2] = 0;
    this->DataExtent[3] = 0;
    this->DataExtent[4] = 0;
    this->DataExtent[5] = 0;
    for( i = 0; i < 3; i++ )
    {
        if( this->DimIndex[i] != -1 )
        {
            this->DataExtent[ 2 * i + 1 ] = this->DimensionLength[ this->DimIndex[i] ] - 1;
        }
    }

    //=============================================
    // Set the number of scalar components
    //=============================================
    this->SetDataScalarType( this->ImageDataType );
    this->SetNumberOfScalarComponents( this->NumberOfScalarComponents );

    //=============================================
    // Let parent class do the rest.
    //=============================================
    this->vtkImageReader2::ExecuteInformation();

    if( this->MincFileId != MI_ERROR )
    {
        miclose( this->MincFileId );
        this->MincFileId = MI_ERROR;
    }
}

//-------------------------------------------------------------------------
// Function for getting VTK dimension index from the dimension name.
int vtkMINCReader::IndexFromDimensionName(const char *dimName)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        if (!strcmp( dimName , this->DimensionNames[i]))
            return this->DimIndex[i];
    }
    return -1;
}
//----------------------------------------------------------------------------
vtkMINCImageAttributes2 *vtkMINCReader::GetImageAttributes()
{
    this->ReadMINCFileAttributes();
    return this->ImageAttributes;
}

//-------------------------------------------------------------------------
// ReadMINCFileAttributes is copied from vtkMINCImageReader by David Gobbi
int vtkMINCReader::ReadMINCFileAttributes() //return ReaderStatus
{
    if( this->MincFileId != MI_ERROR )
    {
        miclose( this->MincFileId );
    }

    this->MincFileId = miopen( this->InternalFileName, NC_NOWRITE );
    if ( this->MincFileId == MI_ERROR )
    {
        vtkErrorMacro(<<"Could not open minc file.");
        this->ReaderStatus = MINC_CORRUPTED;
        return MINC_CORRUPTED;
    }
    // Reset the MINC information for the file.
    int MINCImageType = 0;
    int MINCImageTypeSigned = 1;

    this->DirectionCosinesMatrix->Identity();

    // Orientation set tells us which direction cosines were found
    int orientationSet[3];
    orientationSet[0] = 0;
    orientationSet[1] = 0;
    orientationSet[2] = 0;

    this->ImageAttributes->Reset();

    // Miscellaneous NetCDF variables
    int status = 0;
    int ncid = 0;
    int dimid = 0;
    int varid = 0;
    int ndims = 0;
    int nvars = 0;
    int ngatts = 0;
    int unlimdimid = 0;

    // Get the basic information for the file.  The ndims are
    // ignored here, because we only want the dimensions that
    // belong to the image variable.
    status = nc_inq(this->MincFileId, &ndims, &nvars, &ngatts, &unlimdimid);
    if (status != NC_NOERR)
    {
        this->ReaderStatus = MINC_CORRUPTED;
        miclose( this->MincFileId );
        this->MincFileId = MI_ERROR;
        return MINC_CORRUPTED;
    }
    if (ndims > VTK_MINC_MAX_DIMS)
    {
        vtkErrorMacro("MINC file has " << ndims << ", but this reader"
                        " only supports " << VTK_MINC_MAX_DIMS << ".");
        this->ReaderStatus = MINC_CORRUPTED;
        return MINC_CORRUPTED;
    }

    // Go through all the variables in the MINC file.  A varid of -1
    // is used to signal global attributes.
    for (varid = -1; varid < nvars; varid++)
    {
        char varname[NC_MAX_NAME+1];
        int dimids[VTK_MINC_MAX_DIMS];
        nc_type vartype = NC_SHORT;
        int nvardims = 0;
        int nvaratts = 0;

        if (varid == -1)  // for global attributes
        {
            nvaratts = ngatts;
            varname[0] = '\0';
        }
        else
        {
            status = nc_inq_var(this->MincFileId, varid, varname, &vartype, &nvardims,
                            dimids, &nvaratts);
            if (status != NC_NOERR)
            {
                this->ReaderStatus = MINC_CORRUPTED;
                miclose( this->MincFileId );
                this->MincFileId = MI_ERROR;
                return MINC_CORRUPTED;
            }
        }

        // Get all the variable attributes
        for (int j = 0; j < nvaratts; j++)
        {
            char attname[NC_MAX_NAME+1];
            nc_type atttype;
            size_t attlength = 0;

            status = nc_inq_attname(this->MincFileId, varid, j, attname);
            if (status != NC_NOERR)
            {
                this->ReaderStatus = MINC_CORRUPTED;
                return MINC_CORRUPTED;
            }
            status = nc_inq_att(this->MincFileId, varid, attname, &atttype, &attlength);
            if (status != NC_NOERR)
            {
                this->ReaderStatus = MINC_CORRUPTED;
                return MINC_CORRUPTED;
            }

            // Get the attribute values as a vtkDataArray.
            vtkDataArray *dataArray = 0;
            switch (atttype)
            {
            case NC_BYTE:
                {
                    // NetCDF leaves it up to us to decide whether NC_BYTE
                    // should be signed.
                    vtkUnsignedCharArray *ucharArray = vtkUnsignedCharArray::New();
                    ucharArray->SetNumberOfValues(attlength);
                    nc_get_att_uchar(this->MincFileId, varid, attname,
                                    ucharArray->GetPointer(0));
                    dataArray = ucharArray;
                }
                break;
            case NC_CHAR:
                {
                    // The NC_CHAR type is for text.
                    vtkCharArray *charArray = vtkCharArray::New();
                    // The netcdf standard doesn't enforce null-termination
                    // of string attributes, so we add a null here.
                    charArray->SetNumberOfValues(attlength+1);
                    charArray->SetValue(attlength, 0);
                    charArray->SetNumberOfValues(attlength);
                    nc_get_att_text(this->MincFileId, varid, attname,
                                    charArray->GetPointer(0));
                    dataArray = charArray;
                    }
                break;
            case NC_SHORT:
                {
                    vtkShortArray *shortArray = vtkShortArray::New();
                    shortArray->SetNumberOfValues(attlength);
                    nc_get_att_short(this->MincFileId, varid, attname,
                                    shortArray->GetPointer(0));
                    dataArray = shortArray;
                }
                break;
            case NC_INT:
                {
                    vtkIntArray *intArray = vtkIntArray::New();
                    intArray->SetNumberOfValues(attlength);
                    nc_get_att_int(this->MincFileId, varid, attname,
                                    intArray->GetPointer(0));
                    dataArray = intArray;
                }
                break;
            case NC_FLOAT:
                {
                    vtkFloatArray *floatArray = vtkFloatArray::New();
                    floatArray->SetNumberOfValues(attlength);
                    nc_get_att_float(this->MincFileId, varid, attname,
                                    floatArray->GetPointer(0));
                    dataArray = floatArray;
                }
                break;
            case NC_DOUBLE:
                {
                    vtkDoubleArray *doubleArray = vtkDoubleArray::New();
                    doubleArray->SetNumberOfValues(attlength);
                    nc_get_att_double(this->MincFileId, varid, attname,
                                    doubleArray->GetPointer(0));
                    dataArray = doubleArray;
                }
                break;
            default:
                break;
            }
            if (dataArray)
            {
                this->ImageAttributes->SetAttributeValueAsArray(
                    varname, attname, dataArray);
                dataArray->Delete();
            }
        }

        // Special treatment of image variable.
        if (strcmp(varname, MIimage) == 0)
        {
            // Set the type of the data.
            MINCImageType = vartype;

            // Find the sign of the data, default to "signed"
            int signedType = 1;
            // Except for bytes, where default is "unsigned"
            if (vartype == NC_BYTE)
            {
                signedType = 0;
            }
            const char *signtype = this->ImageAttributes->GetAttributeValueAsString(
                MIimage, MIsigntype);
            if (signtype)
            {
                if (strcmp(signtype, MI_UNSIGNED) == 0)
                {
                    signedType = 0;
                }
            }
            MINCImageTypeSigned = signedType;

            for (int i = 0; i < nvardims; i++)
            {
                char dimname[NC_MAX_NAME+1];
                size_t dimlength = 0;

                dimid = dimids[i];

                status = nc_inq_dim(this->MincFileId, dimid, dimname, &dimlength);
                if (status != NC_NOERR)
                {
                    this->ReaderStatus = MINC_CORRUPTED;
                    return MINC_CORRUPTED;
                }

                this->ImageAttributes->AddDimension(dimname, dimlength);

                int dimIndex = this->IndexFromDimensionName(dimname);

                if (dimIndex >= 0 && dimIndex < 3)
                {
                    // Set the orientation matrix from the direction_cosines
                    vtkDoubleArray *doubleArray =
                    vtkDoubleArray::SafeDownCast(
                        this->ImageAttributes->GetAttributeValueAsArray(
                        dimname, MIdirection_cosines));
                    if (doubleArray && doubleArray->GetNumberOfTuples() == 3)
                    {
                        double *dimDirCos = doubleArray->GetPointer(0);
                        this->DirectionCosinesMatrix->SetElement(dimIndex, 0, dimDirCos[0]);
                        this->DirectionCosinesMatrix->SetElement(dimIndex, 1, dimDirCos[1]);
                        this->DirectionCosinesMatrix->SetElement(dimIndex, 2, dimDirCos[2]);
                        orientationSet[dimIndex] = 1;
                    }
                }
            }
        }
        else if (strcmp(varname, MIimagemin) == 0 ||
                    strcmp(varname, MIimagemax) == 0)
        {
            // Read the image-min and image-max.
            this->ImageAttributes->SetNumberOfImageMinMaxDimensions(nvardims);

            vtkDoubleArray *doubleArray = vtkDoubleArray::New();
            if (strcmp(varname, MIimagemin) == 0)
            {
                this->ImageAttributes->SetImageMin(doubleArray);
            }
            else
            {
                this->ImageAttributes->SetImageMax(doubleArray);
            }
            doubleArray->Delete();

            vtkIdType size = 1;
            size_t start[VTK_MINC_MAX_DIMS];
            size_t count[VTK_MINC_MAX_DIMS];

            for (int i = 0; i < nvardims; i++)
            {
                char dimname[NC_MAX_NAME+1];
                size_t dimlength = 0;

                dimid = dimids[i];

                status = nc_inq_dim(this->MincFileId, dimid, dimname, &dimlength);
                if (status != NC_NOERR)
                {
                    this->ReaderStatus = MINC_CORRUPTED;
                    return MINC_CORRUPTED;
                }

                start[i] = 0;
                count[i] = dimlength;

                size *= dimlength;
            }

            doubleArray->SetNumberOfValues(size);
            status = nc_get_vara_double(this->MincFileId, varid, start, count,
                                        doubleArray->GetPointer(0));
            if (status != NC_NOERR)
            {
                this->ReaderStatus = MINC_CORRUPTED;
                miclose( this->MincFileId );
                this->MincFileId = MI_ERROR;
                return MINC_CORRUPTED;
            }
        }
    }

    // Check to see if only 2 spatial dimensions were included,
    // since we'll have to make up the third dircos if that is the case
    int numDirCos = 0;
    int notSetIndex = 0;
    for (int dcount = 0; dcount < 3; dcount++)
    {
        if (orientationSet[dcount])
        {
            numDirCos++;
        }
        else
        {
            notSetIndex = dcount;
        }
    }
    // If only two were set, use cross product to get the third
    if (numDirCos == 2)
    {
        int idx1 = (notSetIndex + 1) % 3;
        int idx2 = (notSetIndex + 2) % 3;
        double v1[4];
        double v2[4];
        double v3[3];
        for (int tmpi = 0; tmpi < 4; tmpi++)
        {
            v1[tmpi] = v2[tmpi] = 0.0;
        }
        v1[idx1] = 1.0;
        v2[idx2] = 1.0;
        this->DirectionCosinesMatrix->MultiplyPoint(v1, v1);
        this->DirectionCosinesMatrix->MultiplyPoint(v2, v2);
        vtkMath::Cross(v1, v2, v3);
        this->DirectionCosinesMatrix->SetElement(0, notSetIndex, v3[0]);
        this->DirectionCosinesMatrix->SetElement(1, notSetIndex, v3[1]);
        this->DirectionCosinesMatrix->SetElement(2, notSetIndex, v3[2]);
    }

    // Get the data type
    int dataType;
    if( !this->Icv )
    {
        nc_type imageDataType;
        int imageDataTypeIsSigned;
        if( miget_datatype( this->MincFileId, varid, &imageDataType, &imageDataTypeIsSigned ) == MI_ERROR )
        {
            vtkErrorMacro(<<"Could not get image data type");
            this->ReaderStatus = MINC_CORRUPTED;
            miclose( this->MincFileId );
            this->MincFileId = MI_ERROR;
            return MINC_CORRUPTED;
        }
        dataType = vtkMINCIcv::MincDataTypeToVtkDataType( imageDataType, imageDataTypeIsSigned );
    }
    else
    {
        dataType = Icv->GetDataType();
    }
    this->ImageAttributes->SetDataType(dataType);

    // Get the name from the file name by removing the path and
    // the extension.
    const char *fileName = this->FileName;
    char name[128];
    name[0] = '\0';
    int startChar = 0;
    int endChar = static_cast<int>(strlen(fileName));

    for (startChar = endChar-1; startChar > 0; startChar--)
    {
        if (fileName[startChar] == '.')
        {
            endChar = startChar;
        }
        if (fileName[startChar-1] == '/'
#ifdef _WIN32
        || fileName[startChar-1] == '\\'
#endif
        )
        {
            break;
        }
    }
    if (endChar - startChar > 127)
    {
        endChar = startChar + 128;
    }
    if (endChar > startChar)
    {
        strncpy(name, &fileName[startChar], endChar-startChar);
        name[endChar - startChar] = '\0';
    }

    this->ImageAttributes->SetName(name);

    // find global attributes used in ibis
    char varname[2];
    varname[0] = 0;
    if (this->ImageAttributes->HasAttribute(varname, MItime_stamp))
        this->TimeStamp = this->ImageAttributes->GetAttributeValueAsDouble(varname, MItime_stamp);
    if (this->ImageAttributes->HasAttribute(varname, MIcomments))
        strcpy(this->Comment, this->ImageAttributes->GetAttributeValueAsString(varname, MIcomments));
    if( this->MincFileId != MI_ERROR )
    {
        miclose( this->MincFileId );
        this->MincFileId = MI_ERROR;
    }
    return MINC_OK;
}

//----------------------------------------------------------------------------
void vtkMINCReader::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);
    os << indent << "ImageAttributes: " << this->ImageAttributes << "\n";
    if (this->ImageAttributes)
    {
        this->ImageAttributes->PrintSelf(os, indent.GetNextIndent());
    }
    os << indent << "DirectionCosinesMatrix: " << this->DirectionCosinesMatrix << "\n";
    if (this->DirectionCosinesMatrix)
    {
        this->DirectionCosinesMatrix->PrintSelf(os, indent.GetNextIndent());
    }
}

