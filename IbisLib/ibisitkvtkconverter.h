#ifndef IBISITKVTKCONVERTER_H
#define IBISITKVTKCONVERTER_H

#include <itkVTKImageExport.h>
#include <itkImageRegionIterator.h>
#include <itkImage.h>
#include <itkRGBPixel.h>
#include <vtkObject.h>

typedef itk::RGBPixel< unsigned char > RGBPixelType;
typedef itk::Image< RGBPixelType, 3 > IbisRGBImageType;
typedef itk::Image<float,3> IbisItkFloat3ImageType;
typedef itk::Image < unsigned char,3 > IbisItkUnsignedChar3ImageType;
typedef itk::ImageRegionIterator< IbisItkFloat3ImageType > IbisItkFloat3ImageIteratorType;

//// Reimplement origin callback from itk::VTKImageExport to take dir cosines into account correctly
template< class TInputImage >
class IbisItkVTKImageExport : public itk::VTKImageExport< TInputImage >
{
public:
    typedef IbisItkVTKImageExport<TInputImage>   Self;
    typedef itk::SmartPointer< Self >            Pointer;
    typedef TInputImage InputImageType;
    itkNewMacro(Self);
protected:
    typedef typename InputImageType::Pointer    InputImagePointer;
    IbisItkVTKImageExport();
    double * OriginCallback() override;
    double vtkOrigin[3];
};
typedef IbisItkVTKImageExport< IbisItkFloat3ImageType > ItkExporterType;
typedef IbisItkVTKImageExport< IbisRGBImageType > ItkRGBImageExporterType;
typedef IbisItkVTKImageExport< IbisItkUnsignedChar3ImageType > IbisItkUnsignedChar3ExporterType;


class vtkImageImport;
class vtkImageData;
class vtkTransform;
class vtkMatrix4x4;

class IbisItkVtkConverter : public vtkObject
{
public:
    static IbisItkVtkConverter * New() { return new IbisItkVtkConverter; }

    vtkTypeMacro(IbisItkVtkConverter,vtkObject)

    IbisItkVtkConverter();
     virtual ~IbisItkVtkConverter();

    vtkImageData *ConvertItkImageToVtkImage( IbisItkFloat3ImageType::Pointer img, vtkTransform *tr = nullptr );
    vtkImageData *ConvertItkImageToVtkImage(IbisRGBImageType::Pointer img , vtkTransform *tr = nullptr );
    vtkImageData *ConvertItkImageToVtkImage( IbisItkUnsignedChar3ImageType::Pointer img, vtkTransform *tr = nullptr );

    bool ConvertVtkImageToItkImage( IbisItkFloat3ImageType::Pointer itkOutputImage, vtkImageData *image, vtkMatrix4x4 *imageMatrix );
    bool ConvertVtkImageToItkImage( IbisRGBImageType::Pointer itkOutputImage, vtkImageData *image, vtkMatrix4x4 *imageMatrix );
    bool ConvertVtkImageToItkImage( IbisItkUnsignedChar3ImageType::Pointer itkOutputImage, vtkImageData *image, vtkMatrix4x4 *imageMatrix );

protected:

    void BuildVtkImport( itk::VTKImageExportBase * exporter );

    ItkExporterType::Pointer ItkToVtkExporter;
    ItkRGBImageExporterType::Pointer ItkRGBImageToVtkExporter;
    IbisItkUnsignedChar3ExporterType::Pointer ItkToVtkUnsignedChar3lExporter;
    vtkImageImport *ItkToVtkImporter;

private:
    void GetImageTransformFromDirectionCosines( itk::Matrix< double, 3, 3 > dirCosines, vtkTransform *tr );

};

#endif // IBISITKVTKCONVERTER_H
