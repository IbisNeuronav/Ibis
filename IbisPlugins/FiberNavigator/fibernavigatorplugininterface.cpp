/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Francois Rheau for writing this class

#include "fibernavigatorplugininterface.h"

FiberNavigatorPluginInterface::FiberNavigatorPluginInterface()
{
    m_settingsWidget = 0;
    m_fiberObjectId = SceneManager::InvalidId;
    m_maximas = Maximas();
    m_FA = fractionalAnisotropy();
    m_volumeShape = 0;

    //Seed Generation
    m_seedPerAxis = 10;
    GeneratePointInBox(10);

    m_stepSize = 1;
    m_minimumLength = 20;
    m_maximumLength = 200;
    m_maximumAngle = 50;
    m_FAThreshold = 0.25;
    m_inertia = 0.25;
    m_referenceId = 0;
    m_FAMapLoad = false;
    m_useTubes = false;

    //TubeFilter
    m_tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
    m_tubeFilter->SetRadius(.25);
    m_tubeFilter->SetNumberOfSides(8);
    m_tubeFilter->CappingOn();
    m_tubeFilter->Update();

    //Roi related values
    m_roiVisible = false;
    m_firstVisible = true;
    m_roi = vtkBoxWidget2::New();
    m_roi->RotationEnabledOff();
    m_roi->ScalingEnabledOff();
    double bounds[6] = { -10, 10, -10, 10, -10, 10 };
    m_roi->GetRepresentation()->PlaceWidget(bounds);
    
    //Roi Callbacks
    m_roiCallbacks = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    m_roiCallbacks->Connect(m_roi, vtkCommand::InteractionEvent, this, SLOT(RoiModifiedSlot()));
    m_roiCallbacks->Connect(m_roi, vtkCommand::StartInteractionEvent, this, SLOT(RoiModifiedSlot()));
    m_roiCallbacks->Connect(m_roi, vtkCommand::EndInteractionEvent, this, SLOT(RoiModifiedSlot()));
}

FiberNavigatorPluginInterface::~FiberNavigatorPluginInterface()
{
    m_roi->Delete();
}

QString FiberNavigatorPluginInterface::OpenMaximaFile(QString fileName)
{
    m_maximas = Maximas(fileName.toStdString());
    m_maximas.load();

    return fileName;
}
QString FiberNavigatorPluginInterface::OpenFAFile(QString fileName)
{
    m_FA = fractionalAnisotropy(fileName.toStdString());
    m_FA.load();
    m_volumeShape = new int[3];//(m_maximas.GetNbColumns(), m_maximas.GetNbRows(), m_maximas.GetNbFrames());
    m_volumeShape[0]= m_maximas.GetNbColumns();
    m_volumeShape[1]= m_maximas.GetNbRows();
    m_volumeShape[2]= m_maximas.GetNbFrames();

    return fileName;
}
void FiberNavigatorPluginInterface::SceneFinishedLoading()
{
    if(m_settingsWidget)
        m_settingsWidget->UpdateUI();
}

QWidget * FiberNavigatorPluginInterface::CreateTab()
{
    if(GetSceneManager()->GetObjectByID(m_fiberObjectId) == 0)
        CreateObject();

    m_settingsWidget = new FiberObjectSettingsWidget;
    m_settingsWidget->SetPluginInterface(this);

    EnableRoi(m_roiVisible);

    connect( GetSceneManager(), SIGNAL(ObjectAdded(int)), this, SLOT(SceneFinishedLoading()) );
    return m_settingsWidget;
}

bool FiberNavigatorPluginInterface::WidgetAboutToClose()
{
    SceneManager * manager = GetSceneManager();
    Q_ASSERT(manager);
    SceneObject * obj = manager->GetObjectByID(m_fiberObjectId);
    if(obj)
        manager->RemoveObject(obj);
    EnableRoi(false);
    View * v = this->GetSceneManager()->GetMain3DView();
    v->GetOverlayRenderer()->InteractiveOn();

    disconnect( GetSceneManager(), SIGNAL(ObjectAdded(int)), this, SLOT(SceneFinishedLoading()) );
    return true;
}

void FiberNavigatorPluginInterface::SetSeedPerAxis(int nb)
{
    m_seedPerAxis = nb;
    GeneratePointInBox(nb);
    UpdateFibers();
}

void FiberNavigatorPluginInterface::GeneratePointInBox(int nb)
{
    m_pointCloud.clear();
    m_pointCloud.resize(nb*nb*nb);
    int index = 0;
    for(int xIter = 0; xIter < m_seedPerAxis; ++xIter)
        {
            for(int yIter = 0; yIter < m_seedPerAxis; ++yIter)
            {
                for(int zIter = 0; zIter < m_seedPerAxis; ++zIter)
                {
                    int coordinateArray[] = {xIter ,yIter ,zIter};
                    std::vector<int> coordinate(coordinateArray, coordinateArray + sizeof(coordinateArray) / sizeof(int));
                    m_pointCloud[index] = coordinate;
                    ++index;
                }
            }
        }

}

void FiberNavigatorPluginInterface::SetStepSize(double nb)
{
    m_stepSize = nb;
    UpdateFibers();
}

void FiberNavigatorPluginInterface::SetMinimumLength(int nb)
{
    m_minimumLength = nb;
    UpdateFibers();
}

void FiberNavigatorPluginInterface::SetMaximumLength(int nb)
{
    m_maximumLength = nb;
    UpdateFibers();
}

void FiberNavigatorPluginInterface::SetMaximumAngle(int nb)
{
    m_maximumAngle = nb;
    UpdateFibers();
}

void FiberNavigatorPluginInterface::SetFAThreshold(double nb)
{
    m_FAThreshold = nb;
    UpdateFibers();
}

void FiberNavigatorPluginInterface::SetInertia(double nb)
{
    m_inertia = nb;
    UpdateFibers();
}

void FiberNavigatorPluginInterface::SetReferenceId(int nb)
{
    m_referenceId = nb;
    UpdateFibers();
}

std::vector< std::pair< int, QString> > FiberNavigatorPluginInterface::GetObjectsName()
{
    SceneManager* sceneManager = this->GetSceneManager();
    std::vector< std::pair< int, QString> > potentialObject;
    QList<ImageObject*> objects;
    sceneManager->GetAllImageObjects(objects);
    for(int j=0; j<objects.size(); ++j)
    {
        SceneObject* object = objects.at(j);
        QString objectName = object->GetName();
        potentialObject.push_back(std::make_pair(object->GetObjectID(), objectName));
    }
    return potentialObject;
}

void FiberNavigatorPluginInterface::SetFAMapLoad(bool use)
{
    m_FAMapLoad = use;
    UpdateFibers();
}

void FiberNavigatorPluginInterface::SetUseTubes(bool use)
{
    m_useTubes = use;
    UpdateFibers();
}

void FiberNavigatorPluginInterface::SetRoiVisibility(bool visible)
{
    m_roiVisible = visible;
    EnableRoi(visible);
}

double* FiberNavigatorPluginInterface::GetRoiSpacing()
{
    double* roiBounds = GetRoiBounds();
    double  x = (floor((roiBounds[0]-roiBounds[1])*100.0))/100.0,
            y = (floor((roiBounds[2]-roiBounds[3])*100.0))/100.0,
            z = (floor((roiBounds[4]-roiBounds[5])*100.0))/100.0;

    double* roiSpacing = new double[3];
    roiSpacing[0] = abs(x);roiSpacing[1] = abs(y);roiSpacing[2] = abs(z);

    return roiSpacing;
}

double* FiberNavigatorPluginInterface::GetRoiCenter()
{
    double* roiBounds = GetRoiBounds();
    double  x = (floor((roiBounds[0]+roiBounds[1])*100.0))/200.0,
            y = (floor((roiBounds[2]+roiBounds[3])*100.0))/200.0,
            z = (floor((roiBounds[4]+roiBounds[5])*100.0))/200.0;

    double* roiCenter = new double[3];
    roiCenter[0] = x;roiCenter[1] = y;roiCenter[2] = z;

    return roiCenter;
}

double* FiberNavigatorPluginInterface::GetRoiBounds()
{
    return m_roi->GetRepresentation()->GetBounds();
}

void FiberNavigatorPluginInterface::SetRoiBounds(
        double xCenter, double yCenter, double zCenter,
         double xSpacing, double ySpacing, double zSpacing)
{
    double 	xmin = xCenter - xSpacing,
            xmax = xCenter + xSpacing,
            ymin = yCenter - ySpacing,
            ymax = yCenter + ySpacing,
            zmin = zCenter - zSpacing,
            zmax = zCenter + zSpacing;

    double bounds[] = {xmin, xmax, ymin, ymax, zmin, zmax};
    m_roi->GetRepresentation()->PlaceWidget(bounds);

    View * v = this->GetSceneManager()->GetMain3DView();
    v->NotifyNeedRender();
}

void FiberNavigatorPluginInterface::RoiModifiedSlot()
{
    emit RoiModifiedSignal();
    UpdateFibers();
}

void FiberNavigatorPluginInterface::CreateObject()
{
    SceneManager* manager = GetSceneManager();
    Q_ASSERT(manager);
    View * v = manager->GetMain3DView();
    v->GetOverlayRenderer()->InteractiveOff();
	v->GetOverlayRenderer2()->InteractiveOff();

    vtkSmartPointer<PolyDataObject> fobj = vtkSmartPointer<PolyDataObject>::New();
    fobj->SetName("Fibers");
    UpdateFibers(fobj);
    manager->AddObject(fobj);
    manager->SetCurrentObject(fobj);
    m_fiberObjectId = fobj->GetObjectID();
}

void FiberNavigatorPluginInterface::EnableRoi(bool on)
{
    View * v = this->GetSceneManager()->GetMain3DView();

    if(on)
    {
        m_roi->SetInteractor(v->GetInteractor());
        m_roi->On();
        if(m_firstVisible)
        {
            double* center = GetVolumeCenter();
            SetRoiBounds(center[0],center[1],center[2],10,10,10);
            m_firstVisible = false;
            m_settingsWidget->UpdateUI();
        }
    }
    else
    {
        m_roi->SetInteractor(0);
        m_roi->Off();
    }
    UpdateFibers();
    v->NotifyNeedRender();
}

void FiberNavigatorPluginInterface::UpdateFibers()
{
    PolyDataObject * poly = PolyDataObject::SafeDownCast(this->GetSceneManager()->GetObjectByID(m_fiberObjectId));
    if(poly)
        UpdateFibers(poly);
}

void FiberNavigatorPluginInterface::UpdateFibers(PolyDataObject * obj)
{
    //Data structure initialization
    vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPoints> tractPts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkUnsignedCharArray> colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    colors->SetNumberOfComponents(3);
    colors->SetName("Colors");

    SceneManager* sceneManager = this->GetSceneManager();
    if(IsRoiVisible() && sceneManager->GetNumberOfUserObjects() > 1)
    {
        tractPts->SetNumberOfPoints(m_maximumLength/m_stepSize*(m_seedPerAxis*m_seedPerAxis*m_seedPerAxis));

        //Seed generation depend on the box position and the nbr of seed
        double* RoiBounds = GetRoiBounds();
        double xSpacing = (RoiBounds[0] - RoiBounds[1])/(m_seedPerAxis-1);
        double ySpacing = (RoiBounds[2] - RoiBounds[3])/(m_seedPerAxis-1);
        double zSpacing = (RoiBounds[4] - RoiBounds[5])/(m_seedPerAxis-1);

        //Create a transform that concatenate the initial volume transformation
        //And the interface interactive transformation
        vtkSmartPointer<vtkTransform> dataTransform = prepareTransformation();
        vtkSmartPointer<vtkTransform> dataTransformBufferInv = vtkSmartPointer<vtkTransform>::New();
        vtkMatrix4x4* tempMat = vtkMatrix4x4::New();
        dataTransform->GetMatrix(tempMat);
        dataTransformBufferInv->SetMatrix(tempMat);
        dataTransformBufferInv->Update();
        vtkSmartPointer<vtkLinearTransform> dataInverseTransform = dataTransformBufferInv->GetLinearInverse();
        m_previousInverse = dataInverseTransform;

        int index = 0;
        //Each seeds become a starting point
        for(int iter = 0; iter < m_seedPerAxis*m_seedPerAxis*m_seedPerAxis; ++iter)
        {
            //Streamline must go both way
            for(int orientation = -1; orientation <=1; orientation+=2)
            {
                vtkSmartPointer<vtkPoints> currentStreamLinePts = vtkSmartPointer<vtkPoints>::New();
                currentStreamLinePts->SetNumberOfPoints(m_maximumLength/m_stepSize);

                //Adapt the seed point cloud the the current box, to then generate
                //the streamline in the voxel space
                double* voxel = dataInverseTransform->TransformPoint(RoiBounds[0]-xSpacing*m_pointCloud[iter][0],
                        RoiBounds[2]-ySpacing*m_pointCloud[iter][1],
                        RoiBounds[4]-zSpacing*m_pointCloud[iter][2]);

                int numberOfPointsOnFiber = GenerateStreamLine(currentStreamLinePts, voxel, orientation);

                //Only add valid fibers to the polydata
                AddCurrentStreamLine(lines, tractPts, colors, currentStreamLinePts, numberOfPointsOnFiber, index);
            }
        }

        tractPts->SetNumberOfPoints(index);
        poly->SetPoints(tractPts);
        poly->SetLines(lines);
        poly->GetPointData()->SetScalars(colors);

        //Transform the streamline from the voxel space the the world space
        vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        transformFilter->SetInputData(poly);
        transformFilter->SetTransform(dataTransform);
        transformFilter->Update();
        poly = transformFilter->GetOutput();
    }
    //Colorize the streamline using their 3D orientation(segment coloring)
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(SetObjPolydata(obj, poly));
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    View* viewer = sceneManager->GetMain3DView();
    viewer->GetRenderer()->RemoveActor(m_actor);
    viewer->GetRenderer()->AddActor(actor);
    m_actor = actor;

}

vtkSmartPointer<vtkTransform> FiberNavigatorPluginInterface::prepareTransformation()
{
    vtkSmartPointer<vtkTransform> dataTransform = vtkSmartPointer<vtkTransform>::New();
    SceneManager* sceneManager = this->GetSceneManager();

    //TODO : Find the relation between the reference ID and the #object displayed
    ImageObject* refObj = ImageObject::SafeDownCast(sceneManager->GetObjectByID(m_referenceId));
    double* volumeOrigin = refObj->GetImage()->GetOrigin();

    double voxelSize[3]= { m_maximas.getVoxelSizeX(), m_maximas.getVoxelSizeY(), m_maximas.getVoxelSizeZ() };

    //TODO :

    dataTransform->Concatenate(sceneManager->GetObjectByID(m_referenceId)->GetWorldTransform());
    dataTransform->Scale(voxelSize);
    dataTransform->Translate(volumeOrigin[0]/voxelSize[0], volumeOrigin[1]/voxelSize[1], volumeOrigin[2]/voxelSize[2]);

    return dataTransform;
}

double* FiberNavigatorPluginInterface::GetVolumeCenter()
{
    vtkSmartPointer<vtkTransform> dataTransform = vtkSmartPointer<vtkTransform>::New();
    SceneManager* sceneManager = this->GetSceneManager();

    dataTransform->Concatenate(sceneManager->GetObjectByID(m_referenceId)->GetWorldTransform());

    //TODO : Find the relation between the reference ID and the #object displayed
    ImageObject* refObj = ImageObject::SafeDownCast(sceneManager->GetObjectByID(m_referenceId));
    double* volumeBounds = refObj->GetImage()->GetBounds();

    double  x = (floor((volumeBounds[0]+volumeBounds[1])*100.0))/200.0,
            y = (floor((volumeBounds[2]+volumeBounds[3])*100.0))/200.0,
            z = (floor((volumeBounds[4]+volumeBounds[5])*100.0))/200.0;

    return dataTransform->TransformPoint(x,y,z);
}

int FiberNavigatorPluginInterface::GenerateStreamLine(vtkPoints* currentStreamLine, double* voxel, int orientation){
    Vec3 currentPoint(voxel[0], voxel[1], voxel[2]);
    Vec3 previousDir, previousPoint = currentPoint, nextPoint = currentPoint;

    //The first point is in fact the seed
    currentStreamLine->InsertPoint(0, voxel);
    voxel[0] = floor(voxel[0]);voxel[1] = floor(voxel[1]);voxel[2] = floor(voxel[2]);

    std::vector<float> currentMaxima;
    int numberOfPointsOnFiber = 1;

    //Keep the tracking going if there is no reason to stop
    while(IsInValidVoxel(voxel) &&
          m_stepSize*numberOfPointsOnFiber <= m_maximumLength)
    {

        currentMaxima = m_maximas.getMaximaOrientation(voxel);
        //The streamline generation will immediatly stop if a step is invalid
        if(!TakeNextStep(previousDir, currentMaxima, currentPoint, nextPoint, orientation, numberOfPointsOnFiber))
            break;

        previousPoint = currentPoint;
        currentPoint = nextPoint;
        currentStreamLine->InsertPoint(numberOfPointsOnFiber, nextPoint[0], nextPoint[1], nextPoint[2]);

        voxel[0] = floor(currentPoint[0]);voxel[1] = floor(currentPoint[1]);voxel[2] = floor(currentPoint[2]);
        ++numberOfPointsOnFiber;
    }

    return numberOfPointsOnFiber;
}

bool FiberNavigatorPluginInterface::IsInValidVoxel(double* voxel){
    //Simple validation of the position of a seed/voxel
    if(voxel[0] > 0 && voxel[0] < m_volumeShape[0] &&
            voxel[1] > 0 && voxel[1] < m_volumeShape[1] &&
            voxel[2] > 0 && voxel[2] < m_volumeShape[2] &&
            m_FA.getFractionalAnisotropy(voxel) > m_FAThreshold)
        return true;
    else
        return false;
}

bool FiberNavigatorPluginInterface::TakeNextStep(Vec3 &previousDir, std::vector<float> &currentMaxima, Vec3 &currentPoint, Vec3 &nextPoint, int orientation, int numberOfPointsOnFiber)
{
    Vec3 bestPreviousDir, bestNextPoint;
    double minAngle = 360;

    bool sucess = false;
    for(int i = 0; i < currentMaxima.size(); i+=3)
    {
        Vec3 direction(currentMaxima[i], currentMaxima[i+1], currentMaxima[i+2]);

        if(numberOfPointsOnFiber == 1)
            previousDir = direction*orientation;

        double currentAngle = computeAngleBetweenSegment(previousDir, direction);

        //Will choose the peak with the smallest direction change
        if(currentAngle < m_maximumAngle && currentAngle <  minAngle)
        {
            minAngle = currentAngle;
            direction = (1 - m_inertia)*direction+m_inertia*previousDir;

            bestNextPoint = Vec3(currentPoint + direction.Normalise() * m_stepSize);
            bestPreviousDir = direction;
            sucess = true;
        }
        //The peak may be in the opposite direction, but still valid
        else if(180 - currentAngle < m_maximumAngle && 180 - currentAngle < minAngle)
        {

            minAngle = 180 - currentAngle;
            direction = (1 - m_inertia)*direction-m_inertia*previousDir;

            bestNextPoint = Vec3(currentPoint - direction.Normalise() * m_stepSize);
            bestPreviousDir = -1*direction;
            sucess = true;
        }
        //The next peak may respect the angle criteria
        else
            continue;
    }

    nextPoint = bestNextPoint;
    previousDir = bestPreviousDir;
    return sucess;
}

void FiberNavigatorPluginInterface::AddCurrentStreamLine(vtkCellArray* lines, vtkPoints* tractPts,vtkUnsignedCharArray* colorsArray, vtkPoints* currentStreamLine, int numberOfPointsOnFiber, int &index)
{
    //Conditions that need the streamline to be completly generated
    //to be verify and color to be apply easily
    if(numberOfPointsOnFiber > 1 &&
            m_stepSize*numberOfPointsOnFiber >= m_minimumLength)
    {
        lines->InsertNextCell(numberOfPointsOnFiber);
        unsigned char color[3];
        for(vtkIdType j = 0; j < numberOfPointsOnFiber; ++j)
        {
            double currentPoint[3], nextPoint[3];
            currentStreamLine->GetPoint(j, currentPoint);

            //The final segment will have the same color as the previous one
            if(j < numberOfPointsOnFiber-1)
            {
                currentStreamLine->GetPoint(j+1, nextPoint);
                double diff[3] = {currentPoint[0]-nextPoint[0], currentPoint[1]-nextPoint[1], currentPoint[2]-nextPoint[2]};
                double norm = computeNorm(diff);

                color[0] = abs(255*(diff[0]/norm));
                color[1] = abs(255*(diff[1]/norm));
                color[2] = abs(255*(diff[2]/norm));
            }
            tractPts->InsertPoint(index, currentStreamLine->GetPoint(j));

            colorsArray->InsertNextTypedTuple(color);
            lines->InsertCellPoint(index);

            ++index;
        }

    }
}

vtkSmartPointer<vtkPolyData> FiberNavigatorPluginInterface::SetObjPolydata(PolyDataObject* obj, vtkPolyData* fiber)
{
    if(!m_useTubes)
    {
        obj->SetPolyData(fiber);
        return fiber;
    }
    else
    {
        m_tubeFilter->SetInputData(fiber);
        obj->SetPolyData(m_tubeFilter->GetOutput());
		
		m_tubeFilter->Update();
        return m_tubeFilter->GetOutput();
    }
}

double FiberNavigatorPluginInterface::computeAngleBetweenSegment(Vec3 &PP, Vec3 &CP)
{
    double cosValue = computeDotProduct(PP, CP)/(computeSegmentLength(PP)*computeSegmentLength(CP));

    //Prevent numeric instability
    if(cosValue > 1) cosValue = 1;
    if(cosValue < -1) cosValue = -1;

    double radAngle = acos(cosValue);

    //An invalid ArcCosinus will results in a stopping of the streamline
    if(std::isnan(radAngle))
        return 90;
    else
        return radAngle*180.0/vl_pi;
}

double FiberNavigatorPluginInterface::computeDotProduct(Vec3 &FP, Vec3 &LP)
{
    return FP[0]*LP[0] + FP[1]*LP[1] + FP[2]*LP[2];
}

//Use for verification only, segment length will 'always' be equal to stepSize
double FiberNavigatorPluginInterface::computeSegmentLength(Vec3 &FP)
{
    return sqrt((FP[0]*FP[0] + FP[1]*FP[1] + FP[2]*FP[2]));
}

//Use for segment coloring for [0,1] value
double FiberNavigatorPluginInterface::computeNorm(double vector[3])
{
    return sqrt((vector[0])*(vector[0]) +(vector[1])*(vector[1]) +(vector[2])*(vector[2]));
}
