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

#include "vtkPRISMVolumeMapper.h"

#include "vtk_glew.h"

#include "vtkObjectFactory.h"
#include "vtkCamera.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkExecutive.h"
#include "DrawableTexture.h"
#include "GlslShader.h"
#include "vtkColoredCube.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLRenderWindow.h"

#include <sstream>

const char defaultVolumeContribution[] = "           vec4 volumeSample = texture3D( volumes[volIndex], pos ); \n\
            vec4 transferFuncSample = texture1D( transferFunctions[volIndex], volumeSample.x ); \n\
            sampleRGBA += transferFuncSample;";

const char defaultStopConditionCode[] = "        if( finalColor.a > .99 ) \n            break;";

vtkPRISMVolumeMapper::PerVolume::PerVolume()
    : SavedTextureInput(nullptr), VolumeTextureId(0), Property(nullptr), TranferFunctionTextureId(0), Enabled(true), linearSampling(true)
{
    shaderVolumeContribution = defaultVolumeContribution;
}

vtkStandardNewMacro(vtkPRISMVolumeMapper);

//-----------------------------------------------------------------------------
vtkPRISMVolumeMapper::vtkPRISMVolumeMapper()
{
    this->SampleDistance = 1.0;
    this->MultFactor = 1.0;
    this->TextureSpaceSamplingDenominator = .004;
    this->VolumeBounds[0] = 0.0;
    this->VolumeBounds[1] = 1.0;
    this->VolumeBounds[2] = 0.0;
    this->VolumeBounds[3] = 1.0;
    this->VolumeBounds[4] = 0.0;
    this->VolumeBounds[5] = 1.0;
    this->Time = 0.0;
    this->InteractionPoint1[0] = 0.0;
    this->InteractionPoint1[1] = 0.0;
    this->InteractionPoint1[2] = 0.0;
    this->InteractionPoint2[0] = 0.0;
    this->InteractionPoint2[1] = 0.0;
    this->InteractionPoint2[2] = 200.0;
    this->BackfaceTexture = nullptr;
    this->DepthBufferTextureId = 0;
    this->DepthBufferTextureSize[0] = 1;
    this->DepthBufferTextureSize[1] = 1;
    this->WorldToTextureMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    this->VolumeShader = nullptr;
    this->BackfaceShader = nullptr;
    this->VolumeShaderNeedsUpdate = true;
    this->RenderState = nullptr;
    this->ColoredCube = vtkSmartPointer<vtkColoredCube>::New();
    this->StopConditionCode = defaultStopConditionCode;
}

//-----------------------------------------------------------------------------
vtkPRISMVolumeMapper::~vtkPRISMVolumeMapper()
{
}

int vtkPRISMVolumeMapper::IsRenderSupported( vtkVolumeProperty *, vtkRenderer * )
{
    // simtodo : implement this properly
    return 1;
}

class PRISMScopedGLState
{
public:
    PRISMScopedGLState( vtkOpenGLState * s )
        : m_clearColor(s)
        , m_enableDisableBlend(s,GL_BLEND)
        , m_enableDisableCull(s,GL_CULL_FACE)
        , m_enableDisableMultisample(s,GL_MULTISAMPLE)
        , m_blendFuncSeparate(s)
        , m_state(s) {}
    ~PRISMScopedGLState() {}
protected:
    vtkOpenGLState::ScopedglClearColor m_clearColor;
    vtkOpenGLState::ScopedglEnableDisable m_enableDisableBlend;
    vtkOpenGLState::ScopedglEnableDisable m_enableDisableCull;
    vtkOpenGLState::ScopedglEnableDisable m_enableDisableMultisample;
    vtkOpenGLState::ScopedglBlendFuncSeparate m_blendFuncSeparate;
    vtkOpenGLState * m_state;
};

void vtkPRISMVolumeMapper::Render( vtkRenderer * ren, vtkVolume * vol )
{
    // Resets GL error before we start rendering, which helps isolating error within the mapper
    vtkOpenGLCheckErrorMacro("begin vtkPRISMVolumeMapper::Render, " );

    // Keep a copy of the opengl state to be restored
    vtkOpenGLRenderWindow * renWin = vtkOpenGLRenderWindow::SafeDownCast( ren->GetRenderWindow() );
    vtkOpenGLState * ostate = renWin->GetState();
    PRISMScopedGLState scopedState(ostate);

    if( !this->BackfaceShader )
    {
        bool res = this->CreateBackfaceShader();
        if( !res )
        {
            vtkErrorMacro("Could not create backface shader");
            return;
        }
    }

    vtkOpenGLCheckErrorMacro("vtkPRISMVolumeMapper: Failed after backface shader init");

    // Make sure the shader as been created successfully
    if( this->VolumeShaderNeedsUpdate )
    {
        bool res = this->UpdateVolumeShader();
        if( !res )
        {
            vtkErrorMacro("Could not create volume shader.");
            return;
        }
        this->VolumeShaderNeedsUpdate = false;
    }

    vtkOpenGLCheckErrorMacro("vtkPRISMVolumeMapper: Failed after volume shader init");

    // Make sure the volume texture is up to date
    if( !this->UpdateVolumes() )
    {
        return;
    }

    vtkOpenGLCheckErrorMacro("vtkPRISMVolumeMapper: Failed after volume update, " );

    // Send transfer functions to gpu if changed
    if( !this->UpdateTransferFunctions() )
    {
        return;
    }

    vtkOpenGLCheckErrorMacro("vtkPRISMVolumeMapper: Failed after transfer function update, " );

    // Make sure render target textures still correspond to screen size
    if( !this->BackfaceTexture )
    {
        this->BackfaceTexture = DrawableTexture::New();
        this->BackfaceTexture->Init( 1, 1, ostate );
    }

    int renderSize[2];
    GetRenderSize( ren, renderSize );

    this->BackfaceTexture->Resize( renderSize[0], renderSize[1] );

    this->UpdateDepthBufferTexture( renderSize[0], renderSize[1] );

    // Disable multisampling
    ostate->vtkglDisable( GL_MULTISAMPLE );

    ostate->vtkglDisable( GL_BLEND );
    ostate->vtkglDisable( GL_DEPTH_TEST );

    // Draw backfaces to the texture
    ostate->vtkglEnable( GL_CULL_FACE );
    ostate->vtkglCullFace( GL_FRONT );
    BackfaceShader->UseProgram( true );
    bool res = BackfaceShader->SetVariable( "windowSize", renderSize[0], renderSize[1] );
    res &= this->SetCameraMatrices( ren, BackfaceShader );
    res &= BackfaceShader->SetVariable( "volumeMatrix", vol->GetMatrix() );
    BackfaceTexture->DrawToTexture( true );
    ostate->vtkglClearColor( 0.0, 0.0, 0.0, 0.0 );
    ostate->vtkglClear( GL_COLOR_BUFFER_BIT );

    ColoredCube->SetBounds( this->VolumeBounds );
    ColoredCube->SetCropping( this->Cropping );
    ColoredCube->SetCroppingRegionPlanes( this->CroppingRegionPlanes );
    ColoredCube->UpdateGeometry( ren, vol->GetMatrix() );
    ColoredCube->Render();

    BackfaceTexture->DrawToTexture( false );

    vtkOpenGLCheckErrorMacro("vtkPRISMVolumeMapper: Failed after rendering cube backface, " );

    // Draw front of cube and do raycasting in the shader
    ostate->vtkglCullFace( GL_BACK );

    // Bind back texture in texture unit 0
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_RECTANGLE, BackfaceTexture->GetTexId() );

    // Bind depth texture in texture unit 1
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_RECTANGLE, DepthBufferTextureId );

    // Bind all volumes and their respective transfer functions to texture units starting at 1
    for( unsigned i = 0; i < VolumesInfo.size(); ++i )
    {
        glActiveTexture( GL_TEXTURE2 + 2 * i );
        glBindTexture( GL_TEXTURE_1D, VolumesInfo[i].TranferFunctionTextureId );

        glActiveTexture( GL_TEXTURE2 + 2 * i + 1 );
        glBindTexture( GL_TEXTURE_3D, VolumesInfo[i].VolumeTextureId );
        glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, VolumesInfo[i].linearSampling ? GL_LINEAR : GL_NEAREST );
        glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, VolumesInfo[i].linearSampling ? GL_LINEAR : GL_NEAREST );
    }

    // Setup ray-tracer shader program and render front of cube
    VolumeShader->UseProgram( true );

    res &= VolumeShader->SetVariable( "time", Time );
    res &= VolumeShader->SetVariable( "multFactor", this->MultFactor );
    res &= VolumeShader->SetVariable( "back_tex_id", int(0) );
    res &= VolumeShader->SetVariable( "depthBuffer", int(1) );
    res &= VolumeShader->SetVariable( "windowSize", renderSize[0], renderSize[1] );
    res &= VolumeShader->SetVariable( "volumeMatrix", vol->GetMatrix() );

    int * transferFuncTextureUnits = new int[ VolumesInfo.size() ];
    int * volumeTextureUnits = new int[ VolumesInfo.size() ];
    int * volEnabled = new int[ VolumesInfo.size() ];
    for( unsigned i = 0; i < VolumesInfo.size(); ++i )
    {
        transferFuncTextureUnits[i] = 2 * i + 2;
        volumeTextureUnits[i] = 2 * i + 3;
        volEnabled[i] = VolumesInfo[i].Enabled ? 1 : 0;
    }
    res &= VolumeShader->SetVariable( "transferFunctions", VolumesInfo.size(), transferFuncTextureUnits );
    res &= VolumeShader->SetVariable( "volumes", VolumesInfo.size(), volumeTextureUnits );
    res &= VolumeShader->SetVariable( "volOn", VolumesInfo.size(), volEnabled );
    delete [] transferFuncTextureUnits;
    delete [] volumeTextureUnits;
    delete [] volEnabled;

    double realSamplingDistance = this->SampleDistance * this->TextureSpaceSamplingDenominator;
    res &= VolumeShader->SetVariable( "stepSize", float(realSamplingDistance) );
    res &= VolumeShader->SetVariable( "stepSizeAdjustment", float(this->SampleDistance) );
    res &= this->SetEyeTo3DTextureMatrixVariable( vol, ren );
    res &= this->SetCameraMatrices( ren, VolumeShader );

    // compute Interaction point position in 3D texture space
    this->UpdateWorldToTextureMatrix( vol );

    // Compute interaction point position in texture space and pass it to shader
    double interactP1[4];
    interactP1[0] = InteractionPoint1[0]; interactP1[1] = InteractionPoint1[1]; interactP1[2] = InteractionPoint1[2]; interactP1[3] = 1.0;
    double interactP1Trans[4];
    this->WorldToTextureMatrix->MultiplyPoint( interactP1, interactP1Trans );
    res &= VolumeShader->SetVariable( "interactionPoint1", (float)interactP1Trans[0], (float)interactP1Trans[1], (float)interactP1Trans[2] );

    double interactP2[4];
    interactP2[0] = InteractionPoint2[0]; interactP2[1] = InteractionPoint2[1]; interactP2[2] = InteractionPoint2[2]; interactP2[3] = 1.0;
    double interactP2Trans[4];
    this->WorldToTextureMatrix->MultiplyPoint( interactP2, interactP2Trans );
    res &= VolumeShader->SetVariable( "interactionPoint2", (float)interactP2Trans[0], (float)interactP2Trans[1], (float)interactP2Trans[2] );

    // Compute light position in texture space
    double lightPos[4] = { 0.0, 0.0, 0.0, 1.0 };
    vtkLightCollection * lights = ren->GetLights();
    if( lights->GetNumberOfItems() > 0 )
    {
        lights->InitTraversal();
        vtkLight * l = lights->GetNextItem();
        l->GetTransformedPosition( lightPos );
    }
    this->WorldToTextureMatrix->MultiplyPoint( lightPos, lightPos );
    res &= VolumeShader->SetVariable( "lightPosition", (float)lightPos[0], (float)lightPos[1], (float)lightPos[2] );

    // Set cam position and distance range variables in the shader
    res &= SetCameraVariablesInShader( ren, vol );

    ostate->vtkglEnable( GL_BLEND );
    ostate->vtkglBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
    ColoredCube->Render();

    // Unbind all volume and transfer function textures
    for( unsigned i = 0; i < VolumesInfo.size(); ++i )
    {
        glActiveTexture( GL_TEXTURE2 + 2 * i );
        glBindTexture( GL_TEXTURE_1D, 0 );

        glActiveTexture( GL_TEXTURE2 + 2 * i + 1 );
        glBindTexture( GL_TEXTURE_3D, 0 );
    }

    // unbind depth texture in tex unit 1
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_RECTANGLE, 0 );

    // unbind back texture in tex unit 0
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_RECTANGLE, 0 );

    VolumeShader->UseProgram( false );

    // Resets GL error before we start rendering, which helps isolating error within the mapper
    vtkOpenGLCheckErrorMacro("End vtkPRISMVolumeMapper::Render, " );
}

void vtkPRISMVolumeMapper::ReleaseGraphicsResources( vtkWindow * w )
{
    if( BackfaceTexture )
    {
        BackfaceTexture->Delete();
        BackfaceTexture = nullptr;
    }
    if( BackfaceShader )
    {
        BackfaceShader->Delete();
        BackfaceShader = nullptr;
    }
    if( VolumeShader )
    {
        VolumeShader->Delete();
        VolumeShader = nullptr;
        VolumeShaderNeedsUpdate = true;
    }
    if( this->DepthBufferTextureId != 0 )
    {
        glDeleteTextures( 1, &this->DepthBufferTextureId );
        this->DepthBufferTextureId = 0;
    }
    for( int i = 0; i < this->VolumesInfo.size(); ++i )
    {
        PerVolume & pv = this->VolumesInfo[i];
        if( pv.VolumeTextureId != 0 )
            glDeleteTextures( 1, &(pv.VolumeTextureId) );
        pv.VolumeTextureId = 0;
        if( pv.TranferFunctionTextureId != 0 )
            glDeleteTextures( 1, &(pv.TranferFunctionTextureId) );
        pv.TranferFunctionTextureId = 0;
        pv.SavedTextureInput = 0;
    }
    this->ColoredCube->ReleaseGraphicsResources( w );
}

int vtkPRISMVolumeMapper::GetNumberOfInputs()
{
    return VolumesInfo.size();
}

void vtkPRISMVolumeMapper::AddInput( vtkAlgorithmOutput * im, vtkVolumeProperty * property, const char * shaderContrib )
{
    this->AddInputConnection( im );
    PerVolume pv;
    pv.Property = property;
    pv.Property->Register( this );
    pv.shaderVolumeContribution = shaderContrib;
    VolumesInfo.push_back( pv );
    this->VolumeShaderNeedsUpdate = true;
}

void vtkPRISMVolumeMapper::SetShaderInitCode( const char * code )
{
    ShaderInitCode = code;
    this->VolumeShaderNeedsUpdate = true;
}

void vtkPRISMVolumeMapper::SetStopConditionCode( const char * code )
{
    StopConditionCode = code;
    this->VolumeShaderNeedsUpdate = true;
}

void vtkPRISMVolumeMapper::EnableInput( int index, bool enable )
{
    VolumesInfo[index].Enabled = enable;
}

void vtkPRISMVolumeMapper::RemoveInput( int index )
{
    // Remove input
    vtkAlgorithmOutput * algoOut = this->GetInputConnection( 0, index );
    this->RemoveInputConnection( 0, algoOut );

    // Remove internal cache about this volume
    PerVolumeContainer::iterator it = VolumesInfo.begin();
    it += index;
    PerVolume & pv = (*it);
    glDeleteTextures( 1, &pv.VolumeTextureId );
    glDeleteTextures( 1, &pv.TranferFunctionTextureId );
    pv.Property->UnRegister( this );
    VolumesInfo.erase( it );
    this->VolumeShaderNeedsUpdate = true;
}


void vtkPRISMVolumeMapper::ClearAllInputs()
{
    int i = VolumesInfo.size() - 1;
    for( ; i >= 0; --i )
        RemoveInput( i );
    this->VolumeShaderNeedsUpdate = true;
}

void vtkPRISMVolumeMapper::SetUseLinearSampling( int index, bool use )
{
    VolumesInfo[index].linearSampling = use;
}

std::string vtkPRISMVolumeMapper::GetShaderBuildError()
{
    return this->VolumeShader->GetErrorMessage();
}

#include "vtkPRISMVolumeRaycast_FS.h"
#include "vtkPRISMVolumeRaycast_VS.h"
#include "vtkPRISMVolumeMapperBackface_FS.h"
#include "vtkPRISMVolumeMapperBackface_VS.h"

void ReplaceAll( std::string & original, std::string findString, std::string replaceString )
{
    size_t pos = 0;
    while( ( pos = original.find( findString, pos ) ) != std::string::npos )
    {
        original.replace( pos, findString.length(), replaceString );
        pos += replaceString.length();
    }
}

bool vtkPRISMVolumeMapper::CreateBackfaceShader()
{
    if( !this->BackfaceShader )
        this->BackfaceShader = new GlslShader;
    this->BackfaceShader->Reset();
    this->BackfaceShader->AddShaderMemSource( vtkPRISMVolumeMapperBackface_FS );
    this->BackfaceShader->AddVertexShaderMemSource( vtkPRISMVolumeMapperBackface_VS );
    bool result = this->BackfaceShader->Init();
    return result;
}

bool vtkPRISMVolumeMapper::UpdateVolumeShader()
{
    // Replace all occurences of numberOfVolumes in shader code
    std::string shaderCode( vtkPRISMVolumeRaycast_FS );
    std::string nbVolumesFindString( "@NumberOfVolumes@" );
    std::ostringstream os;
    os << VolumesInfo.size();
    ReplaceAll( shaderCode, nbVolumesFindString, os.str() );

    // Put the Custom shader init code in there
    std::string initShaderFindString( "@ShaderInit@" );
    size_t initShaderPos = shaderCode.find( initShaderFindString );
    shaderCode.replace( initShaderPos, initShaderFindString.length(), ShaderInitCode );

    // Replace all occurences of VolumeContributions in shader code
    // Accumulate volume contributions
    std::ostringstream osVolContrib;
    for( unsigned i = 0; i < VolumesInfo.size(); ++i )
    {
        osVolContrib << "        if( volOn[" << i << "] == 1 ) " << std::endl;
        osVolContrib << "        {" << std::endl;
        osVolContrib << "            int volIndex = " << i << ";" << std::endl;
        osVolContrib << VolumesInfo[i].shaderVolumeContribution << std::endl;
        osVolContrib << "        }" << std::endl << std::endl;
    }
    std::string volumeContributionsFindString( "@VolumeContributions@" );
    size_t volContribPos = shaderCode.find( volumeContributionsFindString );
    shaderCode.replace( volContribPos, volumeContributionsFindString.length(), osVolContrib.str() );

    // Replace StopCondition with custom code
    std::string stopConditionFindString( "@StopCondition@" );
    size_t stopConditionPos = shaderCode.find( stopConditionFindString );
    shaderCode.replace( stopConditionPos, stopConditionFindString.length(), StopConditionCode );

    // Build shader
    if( !this->VolumeShader )
        this->VolumeShader = new GlslShader;
    this->VolumeShader->Reset();
    this->VolumeShader->AddShaderMemSource( shaderCode.c_str() );
    this->VolumeShader->AddVertexShaderMemSource( vtkPRISMVolumeRaycast_VS );
    bool result = this->VolumeShader->Init();
    return result;
}

void vtkPRISMVolumeMapper::UpdateWorldToTextureMatrix( vtkVolume * volume )
{
    // Compute texture to volume
    double deltas[3];
    for( int i = 0; i < 3; ++i )
      deltas[i] = this->VolumeBounds[ 2 * i + 1 ] - this->VolumeBounds[ 2 * i ];

    // Compute Texture to volume
    vtkSmartPointer<vtkMatrix4x4> textureToVolume = vtkSmartPointer<vtkMatrix4x4>::New();
    textureToVolume->Zero();
    textureToVolume->SetElement(0,0,deltas[0]);
    textureToVolume->SetElement(1,1,deltas[1]);
    textureToVolume->SetElement(2,2,deltas[2]);
    textureToVolume->SetElement(3,3,1.0);
    textureToVolume->SetElement(0,3,this->VolumeBounds[0]);
    textureToVolume->SetElement(1,3,this->VolumeBounds[2]);
    textureToVolume->SetElement(2,3,this->VolumeBounds[4]);

    // Compute Texture to world
    vtkMatrix4x4 * volumeToWorld = volume->GetMatrix();
    vtkMatrix4x4::Multiply4x4( volumeToWorld, textureToVolume, this->WorldToTextureMatrix );

    // compute world to texture
    this->WorldToTextureMatrix->Invert();
}

//-----------------------------------------------------------------------------
int vtkPRISMVolumeMapper::UpdateVolumes( )
{
    int nbInputs = this->GetNumberOfInputConnections( 0 ); // all connections attached to port 0
    for( int i = 0; i < nbInputs; ++i )
    {
        // Get the image data
        vtkImageData * input = vtkImageData::SafeDownCast( this->GetExecutive()->GetInputData(0, i) );
        if( input )
            this->GetExecutive()->Update();

        PerVolume & pv = this->VolumesInfo[i];
 
        // Has the volume changed in some way?
        bool needToUpdate = false;
        if ( pv.SavedTextureInput != input || ( input && pv.SavedTextureMTime.GetMTime() < input->GetMTime() ) )
            needToUpdate = true;

        if( !needToUpdate )
            continue;

        pv.SavedTextureInput = input;
        pv.SavedTextureMTime.Modified();

        if( !input )
        {
            if( pv.VolumeTextureId != 0 )
                glDeleteTextures( 1, &(pv.VolumeTextureId) );
            pv.VolumeTextureId = 0;
            continue;
        }

        if( input->GetNumberOfScalarComponents() != 1 )
        {
            vtkErrorMacro("Only 1 scalar component supported by this mapper.");
            return 0;
        }

        // Determine texture formats
        int scalarType = input->GetScalarType();
        if( scalarType != VTK_UNSIGNED_CHAR && scalarType != VTK_UNSIGNED_SHORT )
        {
            vtkErrorMacro("Only VTK_UNSIGNED_CHAR and VTK_UNSIGNED_SHORT input scalar type is supported by this mapper");
            return 0;
        }
        GLenum glScalarType = GL_UNSIGNED_BYTE;
        GLint internalFormat = GL_RED;
        if( scalarType == VTK_UNSIGNED_SHORT )
        {
            internalFormat = GL_R16;
            glScalarType = GL_UNSIGNED_SHORT;
        }

        // Check volume texture size is supported
        int dim[3];
        input->GetDimensions(dim);
        glTexImage3D( GL_PROXY_TEXTURE_3D, 0, internalFormat, dim[0], dim[1], dim[2], 0, GL_RED, glScalarType, nullptr );
        GLint texWidth;
        glGetTexLevelParameteriv( GL_PROXY_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &texWidth );
        if( texWidth == 0 )
        {
            vtkErrorMacro("Size of volume is not supported");
            return 0;
        }
        vtkOpenGLCheckErrors( "vtkPRISMPolyDataMapper: Error after checking 3D texture size\n");

        // Set bounds of the volume according to specifications of the first volume (we render only within the bound of the first volume)
        if( i == 0 )
        {
            double start[3];
            input->GetOrigin( start );
            double step[3];
            input->GetSpacing( step );

            double smallestStep = step[0];
            int smallestStepDimIndex = 0;
            for( int i = 0; i < 3; ++i )
            {
                VolumeBounds[2*i] = start[i] - .5 * step[i];
                VolumeBounds[2*i+1] = start[i] + ( (double)dim[i] - .5 ) * step[i];
                if( step[i] < smallestStep )
                {
                    smallestStepDimIndex = i;
                    smallestStep = step[i];
                }
            }

            this->TextureSpaceSamplingDenominator = 1.0 / dim[smallestStepDimIndex];
        }

        // Transfer the input volume to the RGBA volume
        if( pv.VolumeTextureId == 0 )
            glGenTextures( 1, &pv.VolumeTextureId );
        glBindTexture( GL_TEXTURE_3D, pv.VolumeTextureId );
        glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage3D( GL_TEXTURE_3D, 0, internalFormat, dim[0], dim[1], dim[2], 0, GL_RED, glScalarType, input->GetScalarPointer() );
        glBindTexture( GL_TEXTURE_3D, 0 );

        vtkOpenGLCheckErrors( "vtkPRISMPolyDataMapper: Error setting 3D texture for the volume");
    }

    return 1;
}


int vtkPRISMVolumeMapper::UpdateTransferFunctions( )
{
    for( unsigned i = 0; i < VolumesInfo.size(); ++i )
    {
        // Check if the transfer function has changed
        PerVolume & pv = this->VolumesInfo[i];
        bool needUpdate = pv.PropertyMTime.GetMTime() < pv.Property->GetRGBTransferFunctionMTime().GetMTime();
        needUpdate |= pv.PropertyMTime.GetMTime() < pv.Property->GetRGBTransferFunction()->GetMTime();
        needUpdate |= pv.PropertyMTime.GetMTime() < pv.Property->GetScalarOpacityMTime();
        needUpdate |= pv.PropertyMTime.GetMTime() < pv.Property->GetScalarOpacity()->GetMTime();
        needUpdate |= pv.TranferFunctionTextureId == 0;
        if( !needUpdate )
            continue;
        pv.PropertyMTime.Modified();

        // Get Opacity table
        vtkPiecewiseFunction * opacityTransferFunction = pv.Property->GetScalarOpacity();
        int tableSize = 4096;
        double range[2] = { 0.0, 255.0 };
        float * table = new float[tableSize];
        opacityTransferFunction->GetTable( range[0], range[1], tableSize, table );

        // Get color table
        vtkColorTransferFunction * colorTransferFunction = pv.Property->GetRGBTransferFunction();
        float * rgbTable = new float[3*tableSize];
        colorTransferFunction->GetTable( range[0], range[1], tableSize, rgbTable );

        // Merge color and opacity in a single texture
        float * fullTable = new float[4*tableSize];
        for( int j = 0; j < tableSize; ++j )
        {
            fullTable[ 4 * j ]     = rgbTable[ 3 * j ];
            fullTable[ 4 * j + 1 ] = rgbTable[ 3 * j + 1 ];
            fullTable[ 4 * j + 2 ] = rgbTable[ 3 * j + 2 ];
            fullTable[ 4 * j + 3 ] = table[ j ];
        }

        // create texture if needed
        if( pv.TranferFunctionTextureId == 0 )
        {
            glGenTextures( 1, &(pv.TranferFunctionTextureId) );
        }

        glBindTexture( GL_TEXTURE_1D, pv.TranferFunctionTextureId );
        glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexImage1D( GL_TEXTURE_1D, 0, GL_RGBA, tableSize, 0, GL_RGBA, GL_FLOAT, fullTable );
        glBindTexture( GL_TEXTURE_1D, 0 );

        delete [] table;
        delete [] rgbTable;
        delete [] fullTable;

        vtkOpenGLCheckErrorMacro( "Error setting texture for transfer function for volume , " );

    }

    return 1;
}

#include <vtkImageImport.h>
#include <vtkPNGWriter.h>

int vtkPRISMVolumeMapper::UpdateDepthBufferTexture( int width, int height )
{
    // Create texture if it doesn't exist and bind it
    if( this->DepthBufferTextureId == 0 )
    {
        glGenTextures( 1, &this->DepthBufferTextureId );
    }

    glBindTexture( GL_TEXTURE_RECTANGLE, this->DepthBufferTextureId );

    // If size has changed, reallocate texture
    if( width != this->DepthBufferTextureSize[0] || height != this->DepthBufferTextureSize[1] )
    {
        glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_COMPARE_MODE, GL_NONE );
        glTexImage2D( GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0 );
        this->DepthBufferTextureSize[0] = width;
        this->DepthBufferTextureSize[1] = height;
    }

    // Now copy depth buffer to texture
    glReadBuffer( GL_BACK );
    glCopyTexSubImage2D( GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, width, height );

    glBindTexture( GL_TEXTURE_RECTANGLE, 0 );

    return 1;
}

bool vtkPRISMVolumeMapper::SetEyeTo3DTextureMatrixVariable( vtkVolume * volume, vtkRenderer * renderer )
{
    // Compute texture to volume
    double deltas[3];
    for( int i = 0; i < 3; ++i )
      deltas[i] = this->VolumeBounds[ 2 * i + 1 ] - this->VolumeBounds[ 2 * i ];

    vtkSmartPointer<vtkMatrix4x4> datasetToTexture = vtkSmartPointer<vtkMatrix4x4>::New();
    datasetToTexture->Zero();
    datasetToTexture->SetElement(0,0,deltas[0]);
    datasetToTexture->SetElement(1,1,deltas[1]);
    datasetToTexture->SetElement(2,2,deltas[2]);
    datasetToTexture->SetElement(3,3,1.0);
    datasetToTexture->SetElement(0,3,this->VolumeBounds[0]);
    datasetToTexture->SetElement(1,3,this->VolumeBounds[2]);
    datasetToTexture->SetElement(2,3,this->VolumeBounds[4]);
    datasetToTexture->Invert();

    // Compute world to volume
    vtkMatrix4x4 * datasetToWorld = volume->GetMatrix();
    vtkSmartPointer<vtkMatrix4x4> worldToDataset = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMatrix4x4::Invert( datasetToWorld, worldToDataset );

    // Compute world to texture
    vtkSmartPointer<vtkMatrix4x4> worldToTexture = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMatrix4x4::Multiply4x4( worldToDataset, datasetToTexture, worldToTexture );

    // Compute eye to texture
    vtkMatrix4x4 * worldToEye = renderer->GetActiveCamera()->GetViewTransformMatrix();
    vtkSmartPointer<vtkMatrix4x4> eyeToTexture = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMatrix4x4::Multiply4x4( worldToEye, worldToTexture, eyeToTexture );

    // Set the matrix with the shader
    bool res = this->VolumeShader->SetVariable( "eyeToTexture", eyeToTexture );
    return res;
}

#include "vtkOpenGLCamera.h"
#include "vtkMatrix3x3.h"

bool vtkPRISMVolumeMapper::SetCameraMatrices( vtkRenderer * ren, GlslShader * s )
{
    vtkOpenGLCamera * cam = vtkOpenGLCamera::SafeDownCast( ren->GetActiveCamera() );
    if( !ren )
    {
        vtkErrorMacro(<<"vtkPRISMVolumeMapper: this mapper only works with OpenGL.")
        return false;
    }

    vtkMatrix4x4 * glTransformMatrix;
    vtkMatrix4x4 * modelViewMatrix;
    vtkMatrix3x3 * normalMatrix;
    vtkMatrix4x4 * projectionMatrix;
    cam->GetKeyMatrices( ren, modelViewMatrix, normalMatrix, projectionMatrix, glTransformMatrix );

    vtkNew<vtkMatrix4x4> projectionMatrixInverse;
    projectionMatrixInverse->DeepCopy(projectionMatrix);
    projectionMatrixInverse->Invert();

    s->SetVariable( "projectionMatrix", projectionMatrix );
    s->SetVariable( "projectionMatrixInverse", projectionMatrixInverse.Get() );
    s->SetVariable( "modelViewMatrix", modelViewMatrix );

    return true;
}

#include <limits>

bool vtkPRISMVolumeMapper::SetCameraVariablesInShader( vtkRenderer * ren, vtkVolume * volume )
{
    vtkMatrix4x4 * volumeToWorld = volume->GetMatrix();

    // Compute texture space of the camera
    double * camPos3 = ren->GetActiveCamera()->GetPosition();
    double cameraPos[4];
    cameraPos[0] = camPos3[0]; cameraPos[1] = camPos3[1]; cameraPos[2] = camPos3[2]; cameraPos[3] = 1.0;
    this->WorldToTextureMatrix->MultiplyPoint( cameraPos, cameraPos );
    bool res = VolumeShader->SetVariable( "cameraPosition", (float)cameraPos[0], (float)cameraPos[1], (float)cameraPos[2] );

    // Compute camera axis in texture space
    double * camTarget3 = ren->GetActiveCamera()->GetFocalPoint();
    double cameraTarget[4];
    cameraTarget[0] = camTarget3[0]; cameraTarget[1] = camTarget3[1]; cameraTarget[2] = camTarget3[2]; cameraTarget[3] = 1.0;
    this->WorldToTextureMatrix->MultiplyPoint( cameraTarget, cameraTarget );
    double cameraDir[3];
    vtkMath::Subtract( cameraTarget, cameraPos, cameraDir );
    vtkMath::Normalize( cameraDir );

    // Compute the range of distances from the camera
    double minDist = std::numeric_limits<double>::max();
    double maxDist = 0.0;
    for( int x = 0; x < 2; ++x )
    {
        for( int y = 0; y < 2; ++y )
        {
            for( int z = 0; z < 2; ++z )
            {
                double vertex[3] = { 0.0, 0.0, 0.0 };
                vertex[0] = (double)x;
                vertex[1] = (double)y;
                vertex[2] = (double)z;
                double vertexDir[3];
                vtkMath::Subtract( vertex, cameraPos, vertexDir );
                double dist = vtkMath::Dot( vertexDir, cameraDir );
                if( dist < minDist )
                {
                    if( dist < 0.0 )
                        minDist = 0.0;
                    else
                        minDist = dist;
                }
                if( dist > maxDist )
                    maxDist = dist;
            }
        }
    }

    res |= VolumeShader->SetVariable( "volumeDistanceRange", (float)minDist, (float)maxDist );

    return res;
}

void vtkPRISMVolumeMapper::GetRenderSize( vtkRenderer * ren, int size[2] )
{
    if( RenderState )
        RenderState->GetRenderSize( ren, size );
    else
    {
        int * s = ren->GetSize();
        size[0] = s[0];
        size[1] = s[1];
    }
}

#include <vtkInformation.h>

int vtkPRISMVolumeMapper::FillInputPortInformation( int port, vtkInformation * info )
{
    if (!this->Superclass::FillInputPortInformation( port, info ) )
    {
        return 0;
    }

    info->Set( vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set( vtkAlgorithm::INPUT_IS_REPEATABLE(), 1 );
    return 1;
}

//-----------------------------------------------------------------------------
// Print the vtkPRISMVolumeMapper
void vtkPRISMVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sample Distance: " << this->SampleDistance << endl;
}



