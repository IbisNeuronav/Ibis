#ifndef vtkColoredCube_h
#define vtkColoredCube_h

#include <vtkObject.h>

class vtkTessellatedBoxSource;
class vtkClipConvexPolyData;
class vtkPlaneCollection;
class vtkColorPolyData;
class vtkTriangleFilter;
class vtkUnsignedIntArray;
class vtkPolyData;
class vtkRenderer;
class vtkWindow;
class vtkMatrix4x4;

class vtkColoredCube : public vtkObject
{

public:

  static vtkColoredCube *New();
  vtkTypeMacro(vtkColoredCube,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void UpdateGeometry( vtkRenderer * ren, vtkMatrix4x4 * mat );
  void Render();
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Specify the bounding box and cropping region
  vtkGetVector6Macro( Bounds, double );
  vtkSetVector6Macro( Bounds, double );
  vtkGetVector6Macro( CroppingRegionPlanes, double );
  vtkSetVector6Macro( CroppingRegionPlanes, double );

  void SetCropping( int c );
  vtkGetMacro( Cropping, int );
  vtkBooleanMacro( Cropping, int );

protected:

  vtkColoredCube();
  ~vtkColoredCube();

  // Drawing the bounding box
  unsigned Vao;
  unsigned VertexBuffer;
  unsigned ColorBuffer;
  unsigned IndicesBuffer;

  int Cropping;
  double Bounds[6];
  double CroppingRegionPlanes[6];
  vtkTessellatedBoxSource * BoxSource;
  vtkClipConvexPolyData * BoxClip;
  vtkColorPolyData * BoxColoring;
  vtkTriangleFilter * BoxTriangles;
  vtkUnsignedIntArray * BoxIndices;
  vtkPlaneCollection * AllPlanes;
  vtkPlaneCollection * NearFarPlanes;

private:

  vtkColoredCube(const vtkColoredCube&);  // Not implemented.
  void operator=(const vtkColoredCube&);  // Not implemented.
};

#endif

