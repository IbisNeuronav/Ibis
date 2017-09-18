#include "ibisitkvtkconverter.h"
#include "vtkImageImport.h"
#include "vtkImageData.h"

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
    this->ItkToVtkImporter->Delete();
}

vtkImageData * IbisItkVtkConverter::ConvertItkFloat3ImageToVtkImage( IbisItkFloat3ImageType::Pointer img )
{
    this->ItkToVtkExporter = ItkExporterType::New();
    BuildVtkImport( this->ItkToVtkExporter );
    this->ItkToVtkExporter->SetInput( img );
    this->ItkToVtkImporter->Update();
    return this->ItkToVtkImporter->GetOutput();
}

vtkImageData * IbisItkVtkConverter::ConvertItkRGBImageToVtkImage( IbisRGBImageType::Pointer img )
{
    this->ItkRGBImageToVtkExporter = ItkRGBImageExporterType::New();
    BuildVtkImport( this->ItkRGBImageToVtkExporter );
    this->ItkRGBImageToVtkExporter->SetInput( img );
    this->ItkToVtkImporter->Update();
    return this->ItkToVtkImporter->GetOutput();
}

vtkImageData * IbisItkVtkConverter::ConvertItkUnsignedChar3ImageToVtkImage( IbisItkUnsignedChar3ImageType::Pointer img )
{
    this->ItkToVtkUnsignedChar3lExporter = IbisItkUnsignedChar3ExporterType::New();
    BuildVtkImport( this->ItkToVtkUnsignedChar3lExporter );
    this->ItkToVtkUnsignedChar3lExporter->SetInput( img );
    this->ItkToVtkImporter->Update();
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
