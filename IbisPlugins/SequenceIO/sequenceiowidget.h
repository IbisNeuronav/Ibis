// Thanks to Houssem Gueziri for writing this class

#ifndef __SequenceIOWidget_h_
#define __SequenceIOWidget_h_

#include <QWidget>
#include <QRadioButton>

#include <fstream>
#include "trackedsceneobject.h"
#include "ibisitkvtkconverter.h"
#include <vtkTransform.h>
#include <vector>
#include <iostream>
#include <sstream>
#include "vtk_zlib.h"

class SequenceIOPluginInterface;
class USAcquisitionObject;
class QProgressDialog;

namespace Ui
{
    class SequenceIOWidget;
}

class SequenceIOWidget : public QWidget
{

    Q_OBJECT

public:

    explicit SequenceIOWidget(QWidget *parent = nullptr);
    ~SequenceIOWidget();

    void SetPluginInterface( SequenceIOPluginInterface * interf );
    void AddAcquisition(int);
    void RemoveAcquisition(int);

    struct AcqProperties
    {
        QString probeTransformName;
        QStringList trackedTransformNames;
        bool compressed;
        unsigned int compressedDataSize;
        unsigned int imageDimensions[2];
        unsigned int numberOfFrames;
        vtkMatrix4x4 * calibrationMatrix;
        bool isCalibrationFound;
        double timestampBaseline;
        QString elementDataFile;

        AcqProperties()
        {
            compressed = false;
            compressedDataSize = 0;
            imageDimensions[0] = 0;
            imageDimensions[1] = 0;
            numberOfFrames = 0;
            calibrationMatrix = vtkMatrix4x4::New();
            isCalibrationFound = false;
            timestampBaseline = 0;
        }

        bool isValid()
        {
            return imageDimensions[0] && imageDimensions[1] && numberOfFrames 
                && !trackedTransformNames.isEmpty() && !elementDataFile.isEmpty();
        }
    };

private:

    template <typename ImageType> void WriteAcquisition(USAcquisitionObject *, itk::SmartPointer<ImageType>);
    bool ReadAcquisitionMetaData(QString, AcqProperties *&);
    USAcquisitionObject * ReadAcquisitionData(QString, AcqProperties *);

    std::string GetStringFromMatrix(double mat[4][4]);
    template <typename ImageType> void GetMatrixFromImage(double (&mat)[4][4], itk::SmartPointer<ImageType>);
    void GetMatrixFromTransform(double (&mat)[4][4], vtkTransform *);
    void MultiplyMatrix(double(&out)[4][4], double in1[4][4], double in2[4][4]);
    void GetImage(USAcquisitionObject *, itk::SmartPointer<IbisItkUnsignedChar3ImageType> & image, int);
    void GetImage(USAcquisitionObject *, itk::SmartPointer<IbisRGBImageType> & image, int);
    
protected:

    void UpdateUi();

    Ui::SequenceIOWidget * ui;
    SequenceIOPluginInterface * m_pluginInterface;

    std::ofstream m_recordfile;
    QString m_outputFilename;
    QProgressDialog * m_progressBar;
    std::vector<QRadioButton *> m_transformNamesList;
    
    bool m_useMask;
    bool m_applyCalibration;
    AcqProperties * m_acquisitionProperties;

private slots:

    void on_exportButton_clicked();
    void on_useMaskCheckBox_stateChanged(int);

    void on_importButton_clicked();
    void on_openSequenceButton_clicked();

    void UpdateProgress(int);

signals:

    void exportProgressUpdated(int);

};

#endif
