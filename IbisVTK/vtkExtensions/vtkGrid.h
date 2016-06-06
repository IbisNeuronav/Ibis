/*=========================================================================

       Program:   Visualization Toolkit Bic Extension
        Module:    vtkGrid.h

    Anka Kochanowska 
    BIC, MNI, McGill
=========================================================================*/
// .NAME vtkGrid - makes a 2D grid 
// .SECTION Description
// With step, size, start and scale provided by the user it will make a grid
// minimum step 1, maximum 30, default start 0,0, default step 10, scale 1, 
// default size 640x480
// Use either grid actor returned by vtkActor *GetGridActor() (obj macro defined below)
// or vtkPolyData * GetGridPolyData(),
// .SECTION See Also
// 
#ifndef TAG_VTKGRID_H
#define TAG_VTKGRID_H

#include <iostream>
#include "vtkObject.h"
#include "vtkActor.h"

class vtkCamera;
class vtkPolyData;
class vtkPolyDataMapper;

class vtkGrid : public vtkObject
{

public:
    static vtkGrid *New();
    
    vtkTypeRevisionMacro(vtkGrid,vtkObject);
    void PrintSelf(ostream& os, vtkIndent indent);

    vtkGetObjectMacro(GridActor,vtkActor);
    vtkGetObjectMacro(GridPolyData,vtkPolyData);
    
    vtkSetClampMacro( GridStep, int, 1, 30 );
    vtkGetMacro( GridStep, int );
    vtkSetVector2Macro(GridStart, int);     
    vtkGetVector2Macro(GridStart, int);     
    vtkSetVector3Macro(GridScale, double);
    vtkGetVector3Macro(GridScale, double);
    
    virtual void SetDefaultStepAndSize();    
    virtual void SetGridSize(int w, int h);    
    virtual void RestoreDefaultGrid();    
    virtual void MakeGrid();
   

protected:
    int GridStep;
    int GridStart[2];
    int GridSize[2];
    double GridScale[3];
    int DefaultGridStep;
    int DefaultGridStart[2];
    int DefaultGridSize[2];
    double DefaultGridScale[3];
    vtkPolyData * GridPolyData;
    vtkPolyDataMapper * GridMapper;
    vtkActor * GridActor;
    
    vtkGrid( );
    virtual ~vtkGrid();

private:
    vtkGrid(const vtkGrid&);  //Not implemented
    void operator=(const vtkGrid&);  //Not implemented
};


#endif //TAG_VTKGRID_H
