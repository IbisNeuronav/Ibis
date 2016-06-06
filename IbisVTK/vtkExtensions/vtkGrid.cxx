#include "vtkGrid.h"
#include "vtkObjectFactory.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPoints.h"
#include "vtkProperty.h"

const int gridWidth = 640;
const int gridHeight = 480;
const int gridStep = 10;

vtkCxxRevisionMacro(vtkGrid, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkGrid);

vtkGrid::vtkGrid()
{
    GridPolyData = vtkPolyData::New();
    GridMapper = vtkPolyDataMapper::New();
    GridActor = vtkActor::New();
    GridMapper->SetInputData( GridPolyData );
    GridActor->SetMapper( GridMapper );
    GridActor->GetProperty()->SetRepresentationToWireframe();
    GridActor->GetProperty()->SetLineWidth(1.0);
    GridActor->GetProperty()->SetColor(0.9, 0.8, 0.33);
    GridActor->GetProperty()->SetInterpolationToFlat();
    GridActor->GetProperty()->BackfaceCullingOff();
    this->SetDefaultStepAndSize();
    this->MakeGrid();
}

vtkGrid::~vtkGrid()
{
    GridActor->Delete();
    GridPolyData->Delete();
    GridMapper->Delete();
}

void vtkGrid::SetDefaultStepAndSize()
{
    GridStep = gridStep;
    GridStart[0] = 0;
    GridStart[1] = 0;
    GridSize[0] = gridWidth;
    GridSize[1] = gridHeight;
    GridScale[0] = 1.0;
    GridScale[1] = 1.0;
    GridScale[2] = 1.0;
    DefaultGridStep = gridStep;
    DefaultGridStart[0] = 0;
    DefaultGridStart[1] = 0;
    DefaultGridSize[0] = gridWidth;
    DefaultGridSize[1] = gridHeight;
    DefaultGridScale[0] = 1.0;
    DefaultGridScale[1] = 1.0;
    DefaultGridScale[2] = 1.0;
}
    
void vtkGrid::SetGridSize(int w, int h)
{
    GridSize[0] = w;
    GridSize[1] = h;
}
       
void vtkGrid::RestoreDefaultGrid()
{
    this->SetDefaultStepAndSize();
    this->MakeGrid();
}

void vtkGrid::MakeGrid()
{
    int numStepsX, numStepsY, numberOfPoints;
    numStepsX = GridSize[0] / GridStep;
    numStepsY = GridSize[1] / GridStep;
    numberOfPoints = (numStepsX + 1) * 2 + (numStepsY + 1) * 2;
    double incrementX = (double)GridStep * GridScale[0];
    double incrementY = (double)GridStep * GridScale[1];
    vtkPoints * newPoints = vtkPoints::New();
    newPoints->SetNumberOfPoints( numberOfPoints );
    int i = 0;
    int j = 0;
    double coord1;
    for( i = 0; i < numStepsX + 1; i++ )
    {
        coord1 = GridStart[0] + i * incrementX;
        newPoints->SetPoint( 2 * i, coord1, 0.0, 0.0 ); 
        newPoints->SetPoint( 2 * i + 1, coord1, GridSize[1], 0.0 ); 
    }
    for( j = 0; j < numStepsY + 1; j++, i++ )
    {
        coord1 = GridStart[1] + j * incrementY;
        newPoints->SetPoint( 2 * i, 0.0, coord1, 0 ); 
        newPoints->SetPoint( 2 * i + 1, GridSize[0], coord1, 0 );  
    }
    // generate lines
    vtkCellArray * newLines = vtkCellArray::New();
    int sizeLines = newLines->EstimateSize( 2, numStepsX + numStepsY + 4 );
    newLines->Allocate( sizeLines );
    for ( i = 0; i < ( numStepsX + numStepsY+ 4 ); i++ )
    {
        newLines->InsertNextCell( 2 );
        newLines->InsertCellPoint( 2 * i );
        newLines->InsertCellPoint( 2 * i + 1 );
    }
    
    // create the poly data
    GridPolyData->Initialize();
    GridPolyData->SetPoints( newPoints );
    GridPolyData->SetLines( newLines );
    
    // clean up unused lines and points
    newPoints->Delete();
    newLines->Delete();  
}

void vtkGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Step: " << GridStep << std::endl;
  os << indent << "Start: " << GridStart[0] << ", " << GridStart[1] << std::endl;
  os << indent << "Size: " << GridSize[0] << ", " << GridSize[1] << std::endl;
  os << indent << "Scale: " << GridScale[0] << ", " << GridScale[1] << std::endl;
}

