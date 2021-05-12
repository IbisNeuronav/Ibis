// Thanks to Houssem Gueziri for writing this class

#include "sequenceioplugininterface.h"
#include "sequenceiowidget.h"
#include "ui_sequenceiowidget.h"
#include "ibisapi.h"
#include "scenemanager.h"
#include "usacquisitionobject.h"

#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <typeinfo>

#include <QApplication>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QSpacerItem>
#include <QWidgetItem>
#include <QFileDialog>
#include <QFile>
#include <QProgressDialog>

#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkDataArray.h>
#include <vtkPassThrough.h>
#include <itkSmartPointer.h>
#include <itkMetaDataObject.h>
#include <itkMetaDataDictionary.h>

SequenceIOWidget::SequenceIOWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SequenceIOWidget),
    m_pluginInterface(nullptr),
    m_useMask(false),
    m_progressBar(nullptr)
{
    m_acquisitionProperties = new AcqProperties;
    m_outputFilename = "";
    ui->setupUi(this);
    setWindowTitle( "Export Ultrasound Sequence" );
}

SequenceIOWidget::~SequenceIOWidget()
{
    if( m_recordfile.is_open() )
        m_recordfile.close();
    delete ui;
}

void SequenceIOWidget::SetPluginInterface( SequenceIOPluginInterface * interf )
{
    m_pluginInterface = interf;
    UpdateUi();
}

template <typename ImageType>
void SequenceIOWidget::WriteAcquisition(USAcquisitionObject * usAcquisitionObject, itk::SmartPointer<ImageType> image)
{
    m_recordfile.open(m_outputFilename.toUtf8().constData(), std::ios::out | std::ios::binary);
    if( m_recordfile )
    {
        int frameCount = usAcquisitionObject->GetNumberOfSlices();

        m_recordfile << "ObjectType = Image" << std::endl;
        m_recordfile << "NDims = 3" << std::endl;
        m_recordfile << "AnatomicalOrientation = RAS" << std::endl;
        m_recordfile << "BinaryData = True" << std::endl;
        m_recordfile << "BinaryDataByteOrderMSB = False" << std::endl;
        m_recordfile << "CenterOfRotation = 0 0 0" << std::endl;
        // TODO: add write compressed files
        m_recordfile << "CompressedData = False" << std::endl;
        m_recordfile << "DimSize = " << usAcquisitionObject->GetSliceWidth() << " "
            << usAcquisitionObject->GetSliceHeight() << " "
            << frameCount << std::endl;
        m_recordfile << "Kinds = domain domain list" << std::endl;
        m_recordfile << "ElementSpacing = 1 1 1" << std::endl;
        m_recordfile << "ElementType = MET_UCHAR" << std::endl;
        m_recordfile << "Offset = 0 0 0" << std::endl;
        m_recordfile << "TransformMatrix = 1 0 0 0 1 0 0 0 1" << std::endl;
        m_recordfile << "UltrasoundImageOrientation = MF" << std::endl;
        // TODO: write RGB images 
        m_recordfile << "UltrasoundImageType = BRIGHTNESS" << std::endl;

        vtkSmartPointer<vtkTransform> calibrationTransform = usAcquisitionObject->GetCalibrationTransform();
        double cal[4][4];
        this->GetMatrixFromTransform(cal, calibrationTransform);
        std::string strcal = this->GetStringFromMatrix(cal);
        m_recordfile << "CalibrationTransform = " << strcal << std::endl;

        // The timestamp of the first frame is used as a baseline
        // this value is stored in `TimestampBaseline`. This is not required, but avoids sone variable overflow
        double timestampBaseline = 0.0;
        timestampBaseline = usAcquisitionObject->GetFrameTimestamp(0);
        m_recordfile << "TimestampBaseline = " << QString::number(timestampBaseline,'f').toUtf8().constData() << std::endl;

        image = ImageType::New();
        
        for( int i = 0; i < frameCount; i++ )
        {
            this->GetImage(usAcquisitionObject, image, i);
            
            double uncalmat[4][4], calmat[4][4];
            this->GetMatrixFromImage(uncalmat, image);
            this->MultiplyMatrix(calmat, uncalmat, cal);

            double timestamp = usAcquisitionObject->GetFrameTimestamp(i);
            if( timestamp > 0 )
            {
                timestamp = timestamp - timestampBaseline;
            }
            else
            {
                timestamp = (double)i;
            }
            
            std::stringstream ss;
            ss << std::setw(10) << std::setfill('0') << i;
            std::string strFrameId = ss.str();
            m_recordfile << "Seq_Frame" + strFrameId + "_FrameNumber = " + strFrameId << std::endl;
            m_recordfile << "Seq_Frame" + strFrameId + "_ImageToProbeTransform = " << strcal << std::endl;
            m_recordfile << "Seq_Frame" + strFrameId + "_ImageToProbeTransformStatus = OK" << std::endl;
            m_recordfile << "Seq_Frame" + strFrameId + "_ProbeToReferenceTransform = " << this->GetStringFromMatrix(uncalmat) << std::endl;
            m_recordfile << "Seq_Frame" + strFrameId + "_ProbeToReferenceTransformStatus = OK" << std::endl;
            m_recordfile << "Seq_Frame" + strFrameId + "_ImageToReferenceTransform = " << this->GetStringFromMatrix(calmat) << std::endl;
            m_recordfile << "Seq_Frame" + strFrameId + "_ImageToReferenceTransformStatus = OK" << std::endl;
            m_recordfile << "Seq_Frame" + strFrameId + "_ImageStatus = OK" << std::endl;
            m_recordfile << "Seq_Frame" + strFrameId + "_Timestamp = " << timestamp << std::endl; 
            m_recordfile << "Seq_Frame" + strFrameId + "_UnfilteredTimestamp = " << timestamp << std::endl;

            emit exportProgressUpdated(i);
        }

        m_recordfile << "ElementDataFile = LOCAL" << std::endl;

        for( int i = 0; i < frameCount; i++ )
        {
            this->GetImage(usAcquisitionObject, image, i);
            ImageType::PixelType * pPixel = image->GetBufferPointer();
            ImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();
            m_recordfile.write((char *)&pPixel[0], sizeof(ImageType::PixelType) * size[0] * size[1] * size[2]);
            
            emit exportProgressUpdated(i + frameCount);
        }

        m_recordfile.close();
        
    }
}

bool SequenceIOWidget::ReadAcquisitionMetaData(QString filename, AcqProperties * &props)
{
    std::ifstream filereader(filename.toUtf8().constData(), std::ios::in | std::ios::binary);
    if( filereader.is_open() )
    {   
        std::string line;
        while( std::getline(filereader, line) )
        {
            QString qline(line.c_str());
            qline = qline.trimmed();
            QStringList tokens = qline.split("=");
            if( tokens[0].trimmed().contains("ObjectType") )
            {
                if( !tokens[1].trimmed().contains("Image") )
                    return false;
            } 
            else if( tokens[0].trimmed() == "CompressedData" )
            {
                if( tokens[1].trimmed().toLower() == "false" )
                    props->compressed = false;
                else
                    props->compressed = true;
            }
            else if( tokens[0].trimmed() == "CompressedDataSize" )
            {
                props->compressedDataSize = tokens[1].trimmed().toInt();
            }
            else if( tokens[0].trimmed().contains("DimSize") )
            {
                QStringList strdim = tokens[1].trimmed().split(" ");
                props->imageDimensions[0] = strdim[0].toInt();
                props->imageDimensions[1] = strdim[1].toInt();
                props->numberOfFrames = strdim[2].toInt();
                this->StartProgress(props->numberOfFrames, tr("Reading Image MetaData"));
            }
            else if( tokens[0].trimmed().contains("CalibrationTransform") )
            {
                QStringList strtransform = tokens[1].trimmed().split(" ");
                int ii = 0, jj = 0;
                for( int i = 0; i < 16; i++ )
                {
                    ii = i / 4;
                    jj = i % 4;
                    props->calibrationMatrix->SetElement(ii, jj, strtransform[i].toDouble());
                }
                props->isCalibrationFound = true;
            }
            else if( tokens[0].trimmed().contains("TimestampBaseline") )
            {
                props->timestampBaseline = tokens[1].trimmed().toDouble();
            }
            else if( tokens[0].trimmed().startsWith("Seq_") )
            {
                // metadata starting with Seq_ are tracking information (i.e., frame id, frame transform, frame status, timestamp, etc.)
            
                QStringList frameinfo = tokens[0].trimmed().split("_");
                // update progress bar
                if( frameinfo[2] == "FrameNumber" )
                {
                    this->UpdateProgress(tokens[1].toInt());
                }
                // collect unique transform names for GUI display
                else if( frameinfo[2].endsWith("Transform") )
                {
                    // if new transform encountered add to dictionary
                    if( !props->trackedTransformNames.contains(frameinfo[2].trimmed()) )
                        props->trackedTransformNames.append(frameinfo[2].trimmed());
                }
            }
            else if( tokens[0].trimmed().contains("ElementDataFile") )
            {
                //end of metadata
                props->elementDataFile = tokens[1].trimmed();
                break;
            }
        }
        filereader.close();
        this->StopProgress();
    }
    return props->isValid();
}

USAcquisitionObject * SequenceIOWidget::ReadAcquisitionData(QString filename, AcqProperties * props)
{
    std::ifstream filereader(filename.toUtf8().constData(), std::ios::in | std::ios::binary);
    if( filereader.is_open() )
    {
        this->StartProgress(props->numberOfFrames, tr("Reading Image Data..."));

        USAcquisitionObject * usAcquisitionObject = USAcquisitionObject::New();
        QFileInfo fi(filename);
        usAcquisitionObject->SetCanAppendChildren(true);
        usAcquisitionObject->SetNameChangeable(true);
        usAcquisitionObject->SetObjectDeletable(true);
        usAcquisitionObject->SetCanChangeParent(false);
        usAcquisitionObject->SetCanEditTransformManually(true);
        usAcquisitionObject->SetObjectManagedBySystem(false);
        usAcquisitionObject->SetBaseDirectory(fi.dir().absolutePath());
        usAcquisitionObject->SetName(tr("Acquisition_") + fi.baseName());
        usAcquisitionObject->SetCalibrationMatrix(props->calibrationMatrix);
        usAcquisitionObject->SetFrameAndMaskSize(props->imageDimensions[0], props->imageDimensions[1]);

        // list of (transform, status) paris
        vtkSmartPointer<vtkMatrix4x4> * transforms = new vtkSmartPointer<vtkMatrix4x4>[props->numberOfFrames];
        bool * transformStatus = new bool[props->numberOfFrames];
        bool * imageStatus = new bool[props->numberOfFrames];
        double * timestamps = new double[props->numberOfFrames];

        unsigned int transformCount = 0;
        unsigned int transformStatusCount = 0;
        unsigned int imageStatusCount = 0;
        unsigned int timestampsCount = 0;
        
        unsigned int frameId = 0;

        std::string line;
        while( std::getline(filereader, line) )
        {
            QString qline(line.c_str());
            qline = qline.trimmed();
            QStringList tokens = qline.split("=");
            
            if( tokens[0].trimmed().startsWith("Seq_") )
            {
                // metadata starting with Seq_ are tracking information (i.e., frame id, frame transform, frame status, timestamp, etc.)
                QStringList frameinfo = tokens[0].trimmed().split("_");
                std::string readableA = frameinfo[2].toUtf8().constData();
                std::string readableB = props->probeTransformName.toUtf8().constData();

                frameId = frameinfo[1].split("Frame").at(1).toInt();
                if( frameinfo[2] == props->probeTransformName + tr("Status") )
                {
                    // set transform status: only consider OK status
                    transformStatus[frameId] = tokens[1].trimmed() == "OK";
                    transformStatusCount++;
                }
                else if( frameinfo[2] == props->probeTransformName )
                {
                    QStringList strtransform = tokens[1].trimmed().split(" ");
                    transforms[frameId] = vtkMatrix4x4::New();
                    int ii = 0, jj = 0;
                    for( int i = 0; i < 16; i++ )
                    {
                        ii = i / 4;
                        jj = i % 4;
                        transforms[frameId]->SetElement(ii, jj, strtransform[i].toDouble());
                    }
                    transformCount++;
                }
                else if( frameinfo[2] == "Timestamp" )
                {
                    // set timestamp
                    timestamps[frameId] = tokens[1].trimmed().toDouble();
                    timestampsCount++;
                }
                else if( frameinfo[2] == "ImageStatus" )
                {
                    // set transform status: only consider OK status
                    imageStatus[frameId] = tokens[1].trimmed() == "OK";
                    imageStatusCount++;
                }
            }
            else if( tokens[0].trimmed().contains("ElementDataFile") )
            {
                //end of metadata
                break;
            }
        }

        // check if number of frames is the same as frameId
        if( (props->numberOfFrames != frameId + 1) ||
            (props->numberOfFrames != transformCount) ||
            (props->numberOfFrames != transformStatusCount) ||
            (props->numberOfFrames != imageStatusCount) ||
            (props->numberOfFrames != timestampsCount) )
        {
            this->StopProgress();
            filereader.close();
            return nullptr;
        }

        std::vector<unsigned char> allFramesPixelBuffer;
        unsigned int frameSizeInBytes = props->imageDimensions[0] * props->imageDimensions[1] * sizeof(vtkTypeUInt8); // * number of scalar components
        unsigned int allFramesPixelBufferSize = props->numberOfFrames * frameSizeInBytes;
        allFramesPixelBuffer.resize(allFramesPixelBufferSize);

        if( props->compressed )
        {
            unsigned int allFramesCompressedPixelBufferSize = props->compressedDataSize;
            std::vector<unsigned char> allFramesCompressedPixelBuffer;
            allFramesCompressedPixelBuffer.resize(allFramesCompressedPixelBufferSize);

            filereader.read((char *)&allFramesCompressedPixelBuffer[0], allFramesCompressedPixelBufferSize);

            uLongf unCompSize = allFramesPixelBufferSize;
            if( uncompress((Bytef *)&(allFramesPixelBuffer[0]), &unCompSize, (const Bytef *)&(allFramesCompressedPixelBuffer[0]), allFramesCompressedPixelBufferSize) != Z_OK )
            {
                filereader.close();
                this->StopProgress();
                return nullptr;
            }
            if( unCompSize != allFramesPixelBufferSize )
            {
                filereader.close();
                this->StopProgress();
                return nullptr;
            }
        }
        else
        {
            filereader.read((char *)&allFramesPixelBuffer[0], allFramesPixelBufferSize);
        }


        for( int i = 0; i < props->numberOfFrames; i++ )
        {
            vtkImageData * image = vtkImageData::New();
            image->SetDimensions(props->imageDimensions[0], props->imageDimensions[1], 1);
            image->SetExtent(0, props->imageDimensions[0] - 1, 0, props->imageDimensions[1], 0, 0);
            image->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
            unsigned char * pointer = static_cast<unsigned char *>(image->GetScalarPointer());
            
            memcpy(pointer, &allFramesPixelBuffer[0] + i * frameSizeInBytes, frameSizeInBytes);

            if( imageStatus[i] )
            {
                usAcquisitionObject->AddFrame(image, transforms[i], props->timestampBaseline + timestamps[i]);
            }
            this->UpdateProgress(i);
        }
        
        filereader.close();
        this->StopProgress();
        return usAcquisitionObject;
    }
}

void SequenceIOWidget::StartProgress(int max, QString title)
{
    Q_ASSERT(m_pluginInterface);
    IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
    m_progressBar = ibisApi->StartProgress(max, title);
}

void SequenceIOWidget::StopProgress()
{
    Q_ASSERT(m_pluginInterface);
    if( m_progressBar )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        ibisApi->StopProgress(m_progressBar);
        m_progressBar = nullptr;
    }
}

void SequenceIOWidget::UpdateProgress(int i)
{
    Q_ASSERT(m_pluginInterface);
    if( m_progressBar )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        ibisApi->UpdateProgress(m_progressBar, i);
        qApp->processEvents();
        if( m_progressBar->wasCanceled() )
        {
            m_recordfile.close();
            QMessageBox::information(0, "Export Ultrasound Sequence", "Process cancelled", 1, 0);
            return;
        }
    }
}

void SequenceIOWidget::GetImage(USAcquisitionObject * usAcquisitionObject,
                                                itk::SmartPointer<IbisItkUnsignedChar3ImageType> &image, int i)
{
    usAcquisitionObject->GetItkImage(image, i, m_useMask, false);
}

void SequenceIOWidget::GetImage(USAcquisitionObject * usAcquisitionObject,
                                                itk::SmartPointer<IbisRGBImageType> & image, int i)
{
    usAcquisitionObject->GetItkRGBImage(image, i, m_useMask, false);
}

template <typename ImageType>
void SequenceIOWidget::GetMatrixFromImage(double(&mat)[4][4], itk::SmartPointer<ImageType> image)
{
    IbisItkUnsignedChar3ImageType::DirectionType directionCosine = image->GetDirection();
    IbisItkUnsignedChar3ImageType::SpacingType spacing = image->GetSpacing();
    IbisItkUnsignedChar3ImageType::PointType origin = image->GetOrigin();
    std::stringstream strTransform;
    for( int i = 0; i < directionCosine.RowDimensions; ++i )
    {
        for( int j = 0; j < directionCosine.ColumnDimensions; ++j )
        {
            mat[i][j] = directionCosine(i, j) * spacing[i];
        }
        mat[i][3] = origin[i];
    }
    mat[3][0] = 0.0; mat[3][1] = 0.0; mat[3][2] = 0.0; mat[3][3] = 1.0;
}

void SequenceIOWidget::GetMatrixFromTransform(double(&mat)[4][4], vtkTransform * transform)
{
    const double * trArray = transform->GetMatrix()->GetData();
    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        {
            mat[i][j] = trArray[i * 4 + j];
        }
    }
}

void SequenceIOWidget::MultiplyMatrix(double(&out)[4][4], double in1[4][4], double in2[4][4])
{
    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        {
            out[i][j] = 0;
            for( int k = 0; k < 4; k++ )
            {
                out[i][j] += in1[i][k] * in2[k][j];
            }
        }
    }
}

/******************************************************************************************************/

void SequenceIOWidget::UpdateUi()
{
    if (m_pluginInterface)
    {
        IbisAPI * ibisAPI = m_pluginInterface->GetIbisAPI();
        Q_ASSERT(ibisAPI);
        if( ibisAPI )
        {
            ui->usAcquisitionComboBox->clear();

            const SceneManager::ObjectList & allObjects = ibisAPI->GetAllObjects();
            for( int i = 0; i < allObjects.size(); ++i )
            {
                SceneObject * current = allObjects[i];
                if( current != ibisAPI->GetSceneRoot() && current->IsListable() )
                {
                    if( current->IsA("USAcquisitionObject") )
                    {
                        ui->usAcquisitionComboBox->addItem(current->GetName(), QVariant(current->GetObjectID()));
                    }
                }
            }

            if( ui->usAcquisitionComboBox->count() == 0 )
            {
                ui->usAcquisitionComboBox->addItem("None", QVariant(IbisAPI::InvalidId));
            }
        }
    }
    m_useMask = ui->useMaskCheckBox->isChecked();
}

void SequenceIOWidget::UpdateAcquisitionInfoUI()
{
    // clear acquisition info text
    ui->acquisitionInfoLabel->setText("");

    // clear radio buttons
    for( QRadioButton * radbtn : m_transformNamesList )
    {
        ui->inportPropertiesLayout->removeWidget(radbtn);
        delete radbtn;
    }
    m_transformNamesList.clear();

    // set acquisition info text
    if( m_acquisitionProperties->isValid() )
    {
        QString text;
        ui->acquisitionInfoLabel->clear();
        text = tr("Number of frames: %1\nImage dimensions: %2 x %3\nCalibration matrix %4\n")
            .arg(m_acquisitionProperties->numberOfFrames)
            .arg(m_acquisitionProperties->imageDimensions[0])
            .arg(m_acquisitionProperties->imageDimensions[1])
            .arg(m_acquisitionProperties->isCalibrationFound ? tr("found:") : tr("not found:"));
        for( int i = 0; i < 4; i++ )
        {
            for( int j = 0; j < 4; j++ )
            {
                double elem = m_acquisitionProperties->calibrationMatrix->GetElement(i, j);
                text += (elem > 0 ? " " : "") + QString::number(elem, 'f', 4) + "\t";
            }
            text += "\n";
        }
        ui->acquisitionInfoLabel->setText(text);

        // generate radio buttons
        bool recommendedTransformFound = false;
        for( QString transformName : m_acquisitionProperties->trackedTransformNames )
        {
            QRadioButton * radbtn = new QRadioButton(transformName);
            radbtn->setProperty("TransformName", QVariant(transformName));
            if( transformName == tr("ProbeToReferenceTransform") )
            {
                radbtn->setText(transformName + tr(" (Recommended)"));
                radbtn->setChecked(true);
                recommendedTransformFound = true;
            }
            ui->inportPropertiesLayout->addWidget(radbtn);
            m_transformNamesList.push_back(radbtn);
        }

        // set default transform 
        if( (!recommendedTransformFound) & (m_transformNamesList.size() > 0) )
        {
            m_transformNamesList[0]->setChecked(true);
        }
    }
    else
    {
        ui->acquisitionInfoLabel->setText("Invalid US acquisition sequence.");
    }
}

void SequenceIOWidget::on_exportButton_clicked()
{
    Q_ASSERT(m_pluginInterface);
    IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();

    // Get input US acquisition
    if( ui->usAcquisitionComboBox->count() == 0 )
    {
        QMessageBox::information(this, "SequenceIOWidget::on_exportButton_clicked()", "Need to specify an US acquisition.");
        return;
    }
    int usAcquisitionObjectId = ui->usAcquisitionComboBox->itemData(ui->usAcquisitionComboBox->currentIndex()).toInt();
    USAcquisitionObject * usAcquisitionObject;
    if( usAcquisitionObjectId != SceneManager::InvalidId )
    {
        usAcquisitionObject = USAcquisitionObject::SafeDownCast(ibisApi->GetObjectByID(usAcquisitionObjectId));
        Q_ASSERT_X(usAcquisitionObject, "SequenceIOWidget::on_exportButton_clicked()", "Invalid US Acquisition Object");
    }
    else
    {
        QMessageBox::information(this, "SequenceIOWidget::on_exportButton_clicked()", "US acquisition not found.");
        return;
    }

    m_outputFilename = QFileDialog::getSaveFileName(this, tr("Export Ultrasound Sequence"), "", tr("Sequence (*.igs.mha);;All files (*)"));
    if( !m_outputFilename.isEmpty() )
    {
        this->StartProgress(usAcquisitionObject->GetNumberOfSlices() * 2, tr("Ultrasound sequence IO"));
        m_progressBar->setLabelText(tr("Exporting..."));
        connect(this, SIGNAL(exportProgressUpdated(int)), this, SLOT(UpdateProgress(int)));

        if( usAcquisitionObject->GetAcquisitionColor() == ACQ_COLOR_RGB )
        {
            IbisRGBImageType::Pointer image;
            this->WriteAcquisition<IbisRGBImageType>(usAcquisitionObject, image);
        }
        else
        {
            IbisItkUnsignedChar3ImageType::Pointer image;
            this->WriteAcquisition<IbisItkUnsignedChar3ImageType>(usAcquisitionObject, image);
        }
        disconnect(this, SIGNAL(exportProgressUpdated(int)), this, SLOT(UpdateProgress(int)));
        this->StopProgress();
    }
}

void SequenceIOWidget::on_useMaskCheckBox_stateChanged(int state)
{
    m_useMask = (bool)state;
}

void SequenceIOWidget::on_openSequenceButton_clicked()
{
    Q_ASSERT(m_pluginInterface);
    IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();

    QString filename = QFileDialog::getOpenFileName(this, tr("Open Sequence File"), ibisApi->GetSceneDirectory(),
                                                    "Sequence file (*.igs.mha);; All files(*)");
    
    QFileInfo fi(filename);
    if( fi.isFile() )
    {
        // clear acquisition properties
        delete m_acquisitionProperties;
        m_acquisitionProperties = new AcqProperties;

        QString filepath = fi.absoluteFilePath();
        ui->openSequenceEdit->setText(filepath);

        if( this->ReadAcquisitionMetaData(filepath, m_acquisitionProperties) )
        {   
            ui->loadCalibrationTransformButton->setEnabled(true);
            ui->importButton->setEnabled(true);
        }
        else 
        {
            ui->importButton->setEnabled(false);
            ui->loadCalibrationTransformButton->setEnabled(false);
        }

        this->UpdateAcquisitionInfoUI();
    }
}

void SequenceIOWidget::on_importButton_clicked()
{
    // check which transform is selected as the probe transform
    for( QRadioButton * btn : m_transformNamesList )
    {
        if( btn->isChecked() )
        {
            QVariant transformName = btn->property("TransformName");
            m_acquisitionProperties->probeTransformName = transformName.toString();
        }   
    }
    QString filepath = ui->openSequenceEdit->text();
    USAcquisitionObject * acq = this->ReadAcquisitionData(filepath, m_acquisitionProperties);
    if( acq )
    {
        acq->SetCurrentFrame(0);
        acq->Modified();
        Q_ASSERT(m_pluginInterface);
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        ibisApi->AddObject(acq);
    }
}

void SequenceIOWidget::on_loadCalibrationTransformButton_clicked()
{
    Q_ASSERT(m_pluginInterface);
    IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();

    QString filename = QFileDialog::getOpenFileName(this, tr("Open XFM file"), ibisApi->GetSceneDirectory(), tr("*.xfm"), nullptr, QFileDialog::DontUseNativeDialog);
    if( !filename.isEmpty() )
    {
        QFile OpenFile(filename);
        vtkMatrix4x4 * mat = vtkMatrix4x4::New();
        vtkXFMReader * reader = vtkXFMReader::New();
        if( reader->CanReadFile(filename.toUtf8()) )
        {
            reader->SetFileName(filename.toUtf8());
            reader->SetMatrix(mat);
            reader->Update();
            reader->Delete();
            m_acquisitionProperties->calibrationMatrix->DeepCopy(mat);
            mat->Delete();

            this->UpdateAcquisitionInfoUI();
        }
        else
        {
            reader->Delete();
            mat->Delete();
            return;
        }
    }
}

std::string SequenceIOWidget::GetStringFromMatrix(double mat[4][4])
{
    std::stringstream strTransform;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            strTransform << mat[i][j] << " ";
        }
    }
    return strTransform.str();
}

void SequenceIOWidget::AddAcquisition(int objectId)
{
    Q_ASSERT(m_pluginInterface);

    IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
    SceneObject * sceneObject = ibisApi->GetObjectByID(objectId);
    if( sceneObject->IsA("USAcquisitionObject") )
    {
        if( ui->usAcquisitionComboBox->count() == 0 )
        {
            ui->usAcquisitionComboBox->addItem(sceneObject->GetName(), QVariant(objectId));
        }
        else if( ui->usAcquisitionComboBox->count() == 1 )
        {
            int currentItemId = ui->usAcquisitionComboBox->itemData(ui->usAcquisitionComboBox->currentIndex()).toInt();
            if( currentItemId == IbisAPI::InvalidId )
            {
                ui->usAcquisitionComboBox->clear();
            }
            ui->usAcquisitionComboBox->addItem(sceneObject->GetName(), QVariant(objectId));
        }
        else
        {
            ui->usAcquisitionComboBox->addItem(sceneObject->GetName(), QVariant(objectId));
        }
    }
}

void SequenceIOWidget::RemoveAcquisition(int objectId)
{
    Q_ASSERT(m_pluginInterface);

    IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
    if( ibisApi )
    {
        for( int i = 0; i < ui->usAcquisitionComboBox->count(); ++i )
        {
            int currentItemId = ui->usAcquisitionComboBox->itemData(i).toInt();
            if( currentItemId == objectId )
            {
                ui->usAcquisitionComboBox->removeItem(i);
            }
        }

        if( ui->usAcquisitionComboBox->count() == 0 )
        {
            ui->usAcquisitionComboBox->addItem(tr("None"), QVariant(IbisAPI::InvalidId));
        }
    }
}