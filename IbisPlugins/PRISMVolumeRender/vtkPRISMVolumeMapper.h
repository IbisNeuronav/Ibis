/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class
// .NAME vtkPRISMVolumeMapper - volume render with GLSL Raycast
//
// .SECTION Description
//
// .SECTION Thanks
// Thanks to Simon Drouin at the Montreal Neurological Institute for
// developping this class.
//
// .SECTION see also
// vtkVolumeMapper

#ifndef __vtkPRISMVolumeMapper_h
#define __vtkPRISMVolumeMapper_h

#include "vtkVolumeMapper.h"
#include "vtkSmartPointer.h"
#include <vector>
#include <string>

class vtkImageData;
class vtkColorTransferFunction;
class vtkPiecewiseFunction;
class vtkVolumeProperty;
class vtkRenderWindow;
class DrawableTexture;
class GlslShader;
class vtkColoredCube;

class vtkIbisRenderState
{
public:
    virtual void GetRenderSize( vtkRenderer * ren, int size[2] ) = 0;
};

class vtkPRISMVolumeMapper : public vtkVolumeMapper
{

public:

  vtkTypeMacro(vtkPRISMVolumeMapper,vtkVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPRISMVolumeMapper * New();

  // Description:
  // Is hardware rendering supported? No if the input data is
  // more than one independent component, or if the hardware does
  // not support the required extensions
  int IsRenderSupported(vtkVolumeProperty *, vtkRenderer *ren);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol);

  // Description:
  // Release any graphics resources that are being consumed by this texture.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // This is not really a distance. It is a ratio of the sampling
  // distance over the smallest voxel dimension of the first volume
  vtkSetMacro( SampleDistance, float );
  vtkGetMacro( SampleDistance, float );

  // Description:
  // This mapper can take multiple inputs (as opposed to other volume mappers)
  // so it needs these 2 functions to manage inputs. It is still possible to use
  // the SetInput function.
  int GetNumberOfInputs();
  void AddInput( vtkAlgorithmOutput * im, vtkVolumeProperty * property, const char * shaderContrib );
  void SetShaderInitCode( const char * code );
  void SetStopConditionCode( const char * code );
  void EnableInput( int index, bool enable );
  void ClearAllInputs();
  void SetUseLinearSampling( int index, bool use );
  void SetMultFactor( double value ) { this->MultFactor = value; }
  std::string GetShaderBuildError();

  vtkSetMacro( Time, float );
  vtkSetVector3Macro( InteractionPoint1, double );
  vtkSetVector3Macro( InteractionPoint2, double );

  void SetRenderState( vtkIbisRenderState * rs ) { RenderState = rs; }

protected:

  vtkPRISMVolumeMapper();
  ~vtkPRISMVolumeMapper();

  int FillInputPortInformation( int port, vtkInformation * info );

  void RemoveInput( int index );
  
  vtkIbisRenderState * RenderState;
  float        SampleDistance;
  float        MultFactor;
  float        TextureSpaceSamplingDenominator;
  double       VolumeBounds[6];
  float        Time;
  double       InteractionPoint1[3];
  double       InteractionPoint2[3];

  std::string  ShaderInitCode;
  std::string  StopConditionCode;

  // Data about each of the volume
  struct PerVolume
  {
      PerVolume();
      vtkImageData * SavedTextureInput;
      vtkTimeStamp   SavedTextureMTime;
      unsigned       VolumeTextureId;
      vtkVolumeProperty * Property;
      vtkTimeStamp PropertyMTime;
      unsigned TranferFunctionTextureId;
      bool Enabled;
      bool linearSampling;
      std::string shaderVolumeContribution;
  };
  typedef std::vector< PerVolume > PerVolumeContainer;
  PerVolumeContainer VolumesInfo;

  // Graphics resources
  DrawableTexture * BackfaceTexture;
  GlslShader * VolumeShader;
  GlslShader * BackfaceShader;
  bool VolumeShaderNeedsUpdate;
  unsigned DepthBufferTextureId;
  int DepthBufferTextureSize[2];
  vtkSmartPointer<vtkMatrix4x4> WorldToTextureMatrix;
  vtkSmartPointer<vtkColoredCube> ColoredCube;

  // Extension management
  bool GlExtensionsLoaded;
  typedef std::vector< std::string > UnsupportedContainer;
  UnsupportedContainer UnsupportedExtensions;

  bool CreateBackfaceShader();
  bool UpdateVolumeShader();

  // update the matrix that transforms from world coordinate to GL texture coordinates
  void UpdateWorldToTextureMatrix( vtkVolume * volume );

  // Description:
  // Update the internal RGBA representation of the volume. Return 1 if
  // anything change, 0 if nothing changed.
  int UpdateVolumes();

  // Description:
  // build textures that are used in GPU as transfer functions.
  int UpdateTransferFunctions();

  // Description:
  // Create a texture that is used to store content of the depth
  // buffer before volume rendering in order to modify ray entry and exit points
  int UpdateDepthBufferTexture( int width, int height );

  // Description:
  // Compute matrix to transform points from eye (camera) coordinates to
  // texture coordinates. This is needed by the volume shader to translate
  // light position and other params to texture space for shading computation
  bool SetEyeTo3DTextureMatrixVariable( vtkVolume * volume, vtkRenderer * renderer );

  // Description:
  // Compute various camera related variables and set in shader
  // 1 - camera pos in texture space, 2 -the min and max distance
  // between the virtual camera and the volume in texture space.
  bool SetCameraVariablesInShader( vtkRenderer * ren, vtkVolume * volume );

  // Description:
  // Impemented in subclass - check is texture size is OK.
  virtual int IsTextureSizeSupported( int size[3] );

  void LoadExtensions( vtkRenderWindow * window );
  void GetRenderSize( vtkRenderer * ren, int size[2] );

  void CheckGLError( const char * msg );
  
private:

  vtkPRISMVolumeMapper(const vtkPRISMVolumeMapper&);  // Not implemented.
  void operator=(const vtkPRISMVolumeMapper&);  // Not implemented.

};


#endif






