#include "ibisitkvtkconverter.h"
#include "vtkImageImport.h"
#include "vtkImageData.h"
#include "vtkTransform.h"
#include "vtkImageShiftScale.h"
#include "vtkImageLuminance.h"
#include "vtkMath.h"

template< class TInputImage >
IbisItkVTKImageExport< TInputImage >::IbisItkVTKImageExport()
{
    for( int i = 0; i < 3; ++i )
        vtkOrigin[ i ] = 0.0;
}

template< class TInputImage >
double * IbisItkVTKImageExport< TInputImage >::OriginCallback()
{
    // run base class
    double * orig = itk::VTKImageExport< TInputImage >::OriginCallback();

    // Get inverse of the dir cosine matrix
    InputImagePointer input = this->GetInput();
    itk::Matrix< double, 3, 3 > dir_cos = input->GetDirection();
    vnl_matrix_fixed< double, 3, 3 > inv_dir_cos = dir_cos.GetTranspose();

    // Transform the origin back to the way vtk sees it
    vnl_vector_fixed< double, 3 > origin;
    vnl_vector_fixed< double, 3 > o_origin;
    for( int j = 0; j < 3; j++ )
        o_origin[ j ] = orig[ j ];
    origin = inv_dir_cos * o_origin;

    for( int i = 0; i < 3; ++i )
        vtkOrigin[ i ] = origin[ i ];

    return vtkOrigin;
}


IbisItkVtkConverter::IbisItkVtkConverter()
{
    this->ItkToVtkImporter = 0;
}

IbisItkVtkConverter::~IbisItkVtkConverter()
{
    if( ItkToVtkImporter )
        this->ItkToVtkImporter->Delete();
}

vtkImageData * IbisItkVtkConverter::ConvertItkImageToVtkImage(IbisItkFloat3ImageType::Pointer img , vtkTransform *tr = 0)
{
    this->ItkToVtkExporter = ItkExporterType::New();
    BuildVtkImport( this->ItkToVtkExporter );
    this->ItkToVtkExporter->SetInput( img );
    this->ItkToVtkImporter->Update();
    if( tr )
        this->GetImageTransformFromDirectionCosines( img->GetDirection(), tr );
    return this->ItkToVtkImporter->GetOutput();
}

vtkImageData * IbisItkVtkConverter::ConvertItkImageToVtkImage( IbisRGBImageType::Pointer img, vtkTransform *tr = 0 )
{
    this->ItkRGBImageToVtkExporter = ItkRGBImageExporterType::New();
    BuildVtkImport( this->ItkRGBImageToVtkExporter );
    this->ItkRGBImageToVtkExporter->SetInput( img );
    this->ItkToVtkImporter->Update();
    if( tr )
        this->GetImageTransformFromDirectionCosines( img->GetDirection(), tr );
    return this->ItkToVtkImporter->GetOutput();
}

vtkImageData * IbisItkVtkConverter::ConvertItkImageToVtkImage(IbisItkUnsignedChar3ImageType::Pointer img , vtkTransform *tr = 0)
{
    this->ItkToVtkUnsignedChar3lExporter = IbisItkUnsignedChar3ExporterType::New();
    BuildVtkImport( this->ItkToVtkUnsignedChar3lExporter );
    this->ItkToVtkUnsignedChar3lExporter->SetInput( img );
    this->ItkToVtkImporter->Update();
    if( tr )
        this->GetImageTransformFromDirectionCosines( img->GetDirection(), tr );
    return this->ItkToVtkImporter->GetOutput();
}

void IbisItkVtkConverter::BuildVtkImport( itk::VTKImageExportBase * exporter )
{
    this->ItkToVtkImporter = vtkImageImport::New();
    this->ItkToVtkImporter->SetUpdateInformationCallback( exporter->GetUpdateInformationCallback() );
    this->ItkToVtkImporter->SetPipelineModifiedCallback( exporter->GetPipelineModifiedCallback() );
    this->ItkToVtkImporter->SetWholeExtentCallback( exporter->GetWholeExtentCallback() );
    this->ItkToVtkImporter->SetSpacingCallback( exporter->GetSpacingCallback() );
    this->ItkToVtkImporter->SetOriginCallback( exporter->GetOriginCallback() );
    this->ItkToVtkImporter->SetScalarTypeCallback( exporter->GetScalarTypeCallback() );
    this->ItkToVtkImporter->SetNumberOfComponentsCallback( exporter->GetNumberOfComponentsCallback() );
    this->ItkToVtkImporter->SetPropagateUpdateExtentCallback( exporter->GetPropagateUpdateExtentCallback() );
    this->ItkToVtkImporter->SetUpdateDataCallback( exporter->GetUpdateDataCallback() );
    this->ItkToVtkImporter->SetDataExtentCallback( exporter->GetDataExtentCallback() );
    this->ItkToVtkImporter->SetBufferPointerCallback( exporter->GetBufferPointerCallback() );
    this->ItkToVtkImporter->SetCallbackUserData( exporter->GetCallbackUserData() );
}

#include <assert.h>
void IbisItkVtkConverter::GetImageTransformFromDirectionCosines(itk::Matrix< double, 3, 3 > dirCosines, vtkTransform *tr )
{
    assert( tr != 0 );
    vtkMatrix4x4 * rotMat = vtkMatrix4x4::New();
    for( unsigned i = 0; i < 3; ++i )
        for( unsigned j = 0; j < 3; ++j )
            rotMat->SetElement( i, j, dirCosines( i, j ) );
    tr->SetMatrix( rotMat );
    rotMat->Delete();
}

bool IbisItkVtkConverter::ConvertVtkImageToItkImage( IbisItkFloat3ImageType::Pointer itkOutputImage, vtkImageData *slice, vtkMatrix4x4 *calibratedSliceMatrix)
{
    if(! itkOutputImage )
        return false;

    int numberOfScalarComponents = slice->GetNumberOfScalarComponents();
    vtkImageData *grayImage = slice;
    vtkImageLuminance *luminanceFilter = vtkImageLuminance::New();
    if (numberOfScalarComponents > 1)
    {
        luminanceFilter->SetInputData(slice);
        luminanceFilter->Update();
        grayImage = luminanceFilter->GetOutput();
    }
    vtkImageData * image = grayImage;
    vtkImageShiftScale *shifter = vtkImageShiftScale::New();
    if (slice->GetScalarType() != VTK_FLOAT)
    {
        shifter->SetOutputScalarType(VTK_FLOAT);
        shifter->SetClampOverflow(1);
        shifter->SetInputData(grayImage);
        shifter->SetShift(0);
        shifter->SetScale(1.0);
        shifter->Update();
        image = shifter->GetOutput();
    }

    int * dimensions = slice->GetDimensions();
    IbisItkFloat3ImageType::SizeType  size;
    IbisItkFloat3ImageType::IndexType start;
    IbisItkFloat3ImageType::RegionType region;
    const long unsigned int numberOfPixels =  dimensions[0] * dimensions[1] * dimensions[2];
    for (int i = 0; i < 3; i++)
    {
        size[i] = dimensions[i];
    }

    start.Fill(0);
    region.SetIndex( start );
    region.SetSize( size );
    itkOutputImage->SetRegions(region);

    itk::Matrix< double, 3,3 > dirCosine;
    itk::Vector< double, 3 > origin;
    itk::Vector< double, 3 > itkOrigin;
    // set direction cosines
    vtkMatrix4x4 * tmpMat = vtkMatrix4x4::New();
    vtkMatrix4x4::Transpose( calibratedSliceMatrix, tmpMat );
    double step[3], mincStartPoint[3], dirCos[3][3];
    for( int i = 0; i < 3; i++ )
    {
        step[i] = vtkMath::Dot( (*tmpMat)[i], (*tmpMat)[i] );
        step[i] = sqrt( step[i] );
        for( int j = 0; j < 3; j++ )
        {
            dirCos[i][j] = (*tmpMat)[i][j] / step[i];
            dirCosine[j][i] = dirCos[i][j];
        }
    }

    double rotation[3][3];
    vtkMath::Transpose3x3( dirCos, rotation );
    vtkMath::LinearSolve3x3( rotation, (*tmpMat)[3], mincStartPoint );

    for( int i = 0; i < 3; i++ )
        origin[i] =  mincStartPoint[i];
    itkOrigin = dirCosine * origin;
    itkOutputImage->SetSpacing(step);
    itkOutputImage->SetOrigin(itkOrigin);
    itkOutputImage->SetDirection(dirCosine);
    itkOutputImage->Allocate();
    float *itkImageBuffer = itkOutputImage->GetBufferPointer();
    memcpy(itkImageBuffer, image->GetScalarPointer(), numberOfPixels*sizeof(float));
    tmpMat->Delete();
    shifter->Delete();
    luminanceFilter->Delete();
    return true;

}
