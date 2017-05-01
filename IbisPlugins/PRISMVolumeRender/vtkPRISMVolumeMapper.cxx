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
#include "vtkOpenGLExtensionManager.h"
#include "vtkExecutive.h"
#include "DrawableTexture.h"
#include "GlslShader.h"
#include "vtkColoredCube.h"
#include "vtkgl.h"

#include <sstream>

const char defaultVolumeContribution[] = "           vec4 volumeSample = texture3D( volumes[volIndex], pos ); \n\
            vec4 transferFuncSample = texture1D( transferFunctions[volIndex], volumeSample.x ); \n\
            sampleRGBA += transferFuncSample;";

const char defaultStopConditionCode[] = "        if( finalColor.a > .99 ) \n            break;";

vtkPRISMVolumeMapper::PerVolume::PerVolume()
    : SavedTextureInput(0), VolumeTextureId(0), Property(0), TranferFunctionTextureId(0), Enabled(true), linearSampling(true)
{
    shaderVolumeContribution = defaultVolumeContribution;
}

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkPRISMVolumeMapper);
//----------------------------------------------------------------------------

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
    this->BackfaceTexture = 0;
    this->DepthBufferTextureId = 0;
    this->DepthBufferTextureSize[0] = 1;
    this->DepthBufferTextureSize[1] = 1;
    this->WorldToTextureMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    this->VolumeShader = 0;
    this->BackfaceShader = 0;
    this->VolumeShaderNeedsUpdate = true;
    this->GlExtensionsLoaded = false;
    this->RenderState = 0;
    this->ColoredCube = vtkSmartPointer<vtkColoredCube>::New();
    this->StopConditionCode = defaultStopConditionCode;
}

//-----------------------------------------------------------------------------
vtkPRISMVolumeMapper::~vtkPRISMVolumeMapper()
{
}


//-----------------------------------------------------------------------------
vtkPRISMVolumeMapper * vtkPRISMVolumeMapper::New()
{
    return new vtkPRISMVolumeMapper;
}


int vtkPRISMVolumeMapper::IsRenderSupported( vtkVolumeProperty *, vtkRenderer * ren )
{
    // simtodo : implement this properly
    return 1;
}

void vtkPRISMVolumeMapper::CheckGLError( const char * msg )
{
    GLenum res = glGetError();
    if( res != GL_NO_ERROR )
    {
        std::string message( msg );
        message += "GL error: ";
        if( res == GL_INVALID_ENUM )
            message += "GL_INVALID_ENUM";
        else if( res == GL_INVALID_VALUE )
            message += "GL_INVALID_VALUE";
        else if( res == GL_INVALID_OPERATION )
            message += "GL_INVALID_OPERATION";
        else if( res == vtkgl::INVALID_FRAMEBUFFER_OPERATION )
            message += "GL_INVALID_FRAMEBUFFER_OPERATION";
        else if( res == GL_OUT_OF_MEMORY )
            message += "GL_OUT_OF_MEMORY";
        else if( res == GL_STACK_UNDERFLOW )
            message += "GL_STACK_UNDERFLOW";
        else if( res == GL_STACK_OVERFLOW )
            message += "GL_STACK_OVERFLOW";
        else
            message += "Unknown error";
        vtkErrorMacro( << message );
    }
}

void vtkPRISMVolumeMapper::Render( vtkRenderer * ren, vtkVolume * vol )
{
    // Resets GL error before we start rendering, which helps isolating error within the mapper
    CheckGLError( "begin vtkPRISMVolumeMapper::Render, " );

    // Make sure we have all extensions we need
    if( !this->GlExtensionsLoaded )
        this->LoadExtensions( ren->GetRenderWindow() );

    if( !this->GlExtensionsLoaded )
    {
        vtkErrorMacro( "The following extensions are not supported: " );
        for( UnsupportedContainer::iterator it = this->UnsupportedExtensions.begin(); it != this->UnsupportedExtensions.end(); ++it )
            vtkErrorMacro( << (*it) );
        return;
    }

    if( !this->BackfaceShader )
    {
        bool res = this->CreateBackfaceShader();
        if( !res )
        {
            vtkErrorMacro("Could not create backface shader");
            return;
        }
    }

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

    // Make sure the volume texture is up to date
    if( !this->UpdateVolumes() )
    {
        return;
    }

    // Send transfer functions to gpu if changed
    if( !this->UpdateTransferFunctions() )
    {
        return;
    }

    // Make sure render target textures still correspond to screen size
    if( !this->BackfaceTexture )
    {
        this->BackfaceTexture = new DrawableTexture;
        this->BackfaceTexture->Init( 1, 1 );
    }

    int renderSize[2];
    GetRenderSize( ren, renderSize );

    this->BackfaceTexture->Resize( renderSize[0], renderSize[1] );

    this->UpdateDepthBufferTexture( renderSize[0], renderSize[1] );

    GLboolean isMultisampleEnabled = glIsEnabled( GL_MULTISAMPLE );
    if( isMultisampleEnabled )
        glDisable( GL_MULTISAMPLE );

    // Setup volume matrix. Usually, this should be done by the 3D prop, but
    // it seems like it is not the case for volume props.
    double * mat = vol->GetMatrix()->Element[0];
    double mat2[16];
    mat2[0] = mat[0];
    mat2[1] = mat[4];
    mat2[2] = mat[8];
    mat2[3] = mat[12];
    mat2[4] = mat[1];
    mat2[5] = mat[5];
    mat2[6] = mat[9];
    mat2[7] = mat[13];
    mat2[8] = mat[2];
    mat2[9] = mat[6];
    mat2[10] = mat[10];
    mat2[11] = mat[14];
    mat2[12] = mat[3];
    mat2[13] = mat[7];
    mat2[14] = mat[11];
    mat2[15] = mat[15];
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glMultMatrixd( mat2 );

    // Save enable state for blend, lighting, depth test and shade model
    glPushAttrib( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT );

    glDisable( GL_BLEND );
    glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );

    glShadeModel( GL_SMOOTH );

    // Draw backfaces to the texture
    glEnable( GL_CULL_FACE );
    glCullFace( GL_FRONT );
    BackfaceShader->UseProgram( true );
    bool res = BackfaceShader->SetVariable( "windowSize", renderSize[0], renderSize[1] );
    BackfaceTexture->DrawToTexture( true );
    glClearColor( 0.0, 0.0, 0.0, 0.0 );
    glClear( GL_COLOR_BUFFER_BIT );

    ColoredCube->SetBounds( this->VolumeBounds );
    ColoredCube->SetCropping( this->Cropping );
    ColoredCube->SetCroppingRegionPlanes( this->CroppingRegionPlanes );
    ColoredCube->UpdateGeometry( ren, vol->GetMatrix() );
    ColoredCube->Render();

    BackfaceTexture->DrawToTexture( false );

    // Draw front of cube and do raycasting in the shader
    glCullFace( GL_BACK );
    glColor4d( 1.0, 1.0, 1.0, 1.0 );

    // Bind back texture in texture unit 0
    vtkgl::ActiveTexture( GL_TEXTURE0 );
    glEnable( vtkgl::TEXTURE_RECTANGLE_ARB );
    glBindTexture( vtkgl::TEXTURE_RECTANGLE_ARB, BackfaceTexture->GetTexId() );

    // Bind depth texture in texture unit 1
    vtkgl::ActiveTexture( GL_TEXTURE1 );
    glEnable( vtkgl::TEXTURE_RECTANGLE_ARB );
    glBindTexture( vtkgl::TEXTURE_RECTANGLE_ARB, DepthBufferTextureId );

    // Bind all volumes and their respective transfer functions to texture units starting at 1
    for( unsigned i = 0; i < VolumesInfo.size(); ++i )
    {
        vtkgl::ActiveTexture( GL_TEXTURE2 + 2 * i );
        glEnable( GL_TEXTURE_1D );
        glBindTexture( GL_TEXTURE_1D, VolumesInfo[i].TranferFunctionTextureId );

        vtkgl::ActiveTexture( GL_TEXTURE2 + 2 * i + 1 );
        glEnable( GL_TEXTURE_3D );
        glBindTexture( GL_TEXTURE_3D, VolumesInfo[i].VolumeTextureId );
        glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, VolumesInfo[i].linearSampling ? GL_LINEAR : GL_NEAREST );
        glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, VolumesInfo[i].linearSampling ? GL_LINEAR : GL_NEAREST );
    }

    // Setup ray-tracer shader program and render front of cube
    VolumeShader->UseProgram( true );

    res = VolumeShader->SetVariable( "time", Time );
    res = VolumeShader->SetVariable( "multFactor", this->MultFactor );
    res = VolumeShader->SetVariable( "back_tex_id", int(0) );
    res = VolumeShader->SetVariable( "depthBuffer", int(1) );
    res = VolumeShader->SetVariable( "windowSize", renderSize[0], renderSize[1] );

    int * transferFuncTextureUnits = new int[ VolumesInfo.size() ];
    int * volumeTextureUnits = new int[ VolumesInfo.size() ];
    int * volEnabled = new int[ VolumesInfo.size() ];
    for( unsigned i = 0; i < VolumesInfo.size(); ++i )
    {
        transferFuncTextureUnits[i] = 2 * i + 2;
        volumeTextureUnits[i] = 2 * i + 3;
        volEnabled[i] = VolumesInfo[i].Enabled ? 1 : 0;
    }
    res = VolumeShader->SetVariable( "transferFunctions", VolumesInfo.size(), transferFuncTextureUnits );
    res = VolumeShader->SetVariable( "volumes", VolumesInfo.size(), volumeTextureUnits );
    res = VolumeShader->SetVariable( "volOn", VolumesInfo.size(), volEnabled );
    delete [] transferFuncTextureUnits;
    delete [] volumeTextureUnits;
    delete [] volEnabled;

    double realSamplingDistance = this->SampleDistance * this->TextureSpaceSamplingDenominator;
    res = VolumeShader->SetVariable( "stepSize", float(realSamplingDistance) );
    res = VolumeShader->SetVariable( "stepSizeAdjustment", float(this->SampleDistance) );
    res = this->SetEyeTo3DTextureMatrixVariable( vol, ren );

    // compute Interaction point position in 3D texture space
    this->UpdateWorldToTextureMatrix( vol );

    // Compute interaction point position in texture space and pass it to shader
    double interactP1[4];
    interactP1[0] = InteractionPoint1[0]; interactP1[1] = InteractionPoint1[1]; interactP1[2] = InteractionPoint1[2]; interactP1[3] = 1.0;
    double interactP1Trans[4];
    this->WorldToTextureMatrix->MultiplyPoint( interactP1, interactP1Trans );
    res = VolumeShader->SetVariable( "interactionPoint1", (float)interactP1Trans[0], (float)interactP1Trans[1], (float)interactP1Trans[2] );

    double interactP2[4];
    interactP2[0] = InteractionPoint2[0]; interactP2[1] = InteractionPoint2[1]; interactP2[2] = InteractionPoint2[2]; interactP2[3] = 1.0;
    double interactP2Trans[4];
    this->WorldToTextureMatrix->MultiplyPoint( interactP2, interactP2Trans );
    res = VolumeShader->SetVariable( "interactionPoint2", (float)interactP2Trans[0], (float)interactP2Trans[1], (float)interactP2Trans[2] );

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
    res = VolumeShader->SetVariable( "lightPosition", (float)lightPos[0], (float)lightPos[1], (float)lightPos[2] );

    // Set cam position and distance range variables in the shader
    res |= SetCameraVariablesInShader( ren, vol );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
    ColoredCube->Render();

    // retrieve old modelview matrix
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

    glDisable( GL_BLEND );

    // Unbind all volume and transfer function textures
    for( unsigned i = 0; i < VolumesInfo.size(); ++i )
    {
        vtkgl::ActiveTexture( GL_TEXTURE2 + 2 * i );
        glBindTexture( GL_TEXTURE_1D, 0 );
        glDisable( GL_TEXTURE_1D );

        vtkgl::ActiveTexture( GL_TEXTURE2 + 2 * i + 1 );
        glBindTexture( GL_TEXTURE_3D, 0 );
        glDisable( GL_TEXTURE_3D );
    }

    // unbind depth texture in tex unit 1
    vtkgl::ActiveTexture( GL_TEXTURE1 );
    glBindTexture( vtkgl::TEXTURE_RECTANGLE_ARB, 0 );
    glDisable( vtkgl::TEXTURE_RECTANGLE_ARB );

    // unbind back texture in tex unit 0
    vtkgl::ActiveTexture( GL_TEXTURE0 );
    glBindTexture( vtkgl::TEXTURE_RECTANGLE_ARB, 0 );
    glDisable( vtkgl::TEXTURE_RECTANGLE_ARB );

    VolumeShader->UseProgram( false );

    if( isMultisampleEnabled )
        glEnable( GL_MULTISAMPLE );

    // retrieve enable state for blend, lighting and depth test
    glPopAttrib();
}

void vtkPRISMVolumeMapper::ReleaseGraphicsResources( vtkWindow * )
{
    if( BackfaceTexture )
    {
        delete BackfaceTexture;
        BackfaceTexture = 0;
    }
    if( BackfaceShader )
    {
        BackfaceShader->Delete();
        BackfaceShader = 0;
    }
    if( VolumeShader )
    {
        VolumeShader->Delete();
        VolumeShader = 0;
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
    this->GlExtensionsLoaded = false;
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

void ReplaceAll( std::string & original, std::string findString, std::string replaceString )
{
    size_t pos = 0;
    while( ( pos = original.find( findString, pos ) ) != std::string::npos )
    {
        original.replace( pos, findString.length(), replaceString );
        pos += replaceString.length();
    }
}

const char backfaceShaderCode[] = "uniform ivec2 windowSize; \
        void main() \
        { \
            gl_FragColor = gl_Color; \
            vec4 ndcPos;  \
            ndcPos.xy = ( (gl_FragCoord.xy / vec2(windowSize) ) * 2.0) - 1.0; \
            ndcPos.z = (2.0 * gl_FragCoord.z - gl_DepthRange.near - gl_DepthRange.far) / gl_DepthRange.diff; \
            ndcPos.w = 1.0; \
            vec4 clipPos = ndcPos / gl_FragCoord.w; \
            vec4 eyeSpaceCoord = gl_ProjectionMatrixInverse * clipPos; \
            gl_FragColor.a =  -eyeSpaceCoord.z; \
        }";

bool vtkPRISMVolumeMapper::CreateBackfaceShader()
{
    std::string shaderCode( backfaceShaderCode );
    if( !this->BackfaceShader )
        this->BackfaceShader = new GlslShader;
    this->BackfaceShader->Reset();
    this->BackfaceShader->AddShaderMemSource( shaderCode.c_str() );
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
    vtkMatrix4x4::Multiply4x4( volumeToWorld, textureToVolume.GetPointer(), this->WorldToTextureMatrix.GetPointer() );

    // compute world to texture
    this->WorldToTextureMatrix->Invert();
}

int vtkPRISMVolumeMapper::IsTextureSizeSupported( int size[3] )
{
    glTexImage3D( GL_PROXY_TEXTURE_3D, 0, GL_LUMINANCE, size[0], size[1], size[2], 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL );

    GLint width;
    glGetTexLevelParameteriv( GL_PROXY_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &width );

    if( width != 0 )
        return 1;
    return 0;
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

        int dim[3];
        input->GetDimensions(dim);
        if( !this->IsTextureSizeSupported( dim ) )
        {
            vtkErrorMacro("Size of volume is not supported");
            return 0;
        }

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

        int scalarType = input->GetScalarType();
        if( scalarType != VTK_UNSIGNED_CHAR && scalarType != VTK_UNSIGNED_SHORT )
        {
            vtkErrorMacro("Only VTK_UNSIGNED_CHAR and VTK_UNSIGNED_SHORT input scalar type is supported by this mapper");
            return 0;
        }
        GLenum glScalarType = GL_UNSIGNED_BYTE;
        GLint internalFormat = GL_LUMINANCE;
        if( scalarType == VTK_UNSIGNED_SHORT )
        {
            internalFormat = GL_LUMINANCE16;
            glScalarType = GL_UNSIGNED_SHORT;
        }

        // Transfer the input volume to the RGBA volume
        glEnable( GL_TEXTURE_3D );
        if( pv.VolumeTextureId == 0 )
            glGenTextures( 1, &pv.VolumeTextureId );
        glBindTexture( GL_TEXTURE_3D, pv.VolumeTextureId );
        //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage3D( GL_TEXTURE_3D, 0, internalFormat, dim[0], dim[1], dim[2], 0, GL_LUMINANCE, glScalarType, input->GetScalarPointer() );
        glBindTexture( GL_TEXTURE_3D, 0 );
        glDisable( GL_TEXTURE_3D );

        if( glGetError() != GL_NO_ERROR )
        {
            vtkErrorMacro( "Error setting 3D texture for the volume" );
            return 0;
        }
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
        glEnable( GL_TEXTURE_1D );
        glBindTexture( GL_TEXTURE_1D, pv.TranferFunctionTextureId );
        glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE );
        glTexImage1D( GL_TEXTURE_1D, 0, 4, tableSize, 0, GL_RGBA, GL_FLOAT, fullTable );
        glBindTexture( GL_TEXTURE_1D, 0 );
        glDisable( GL_TEXTURE_1D );

        delete [] table;
        delete [] rgbTable;
        delete [] fullTable;

        if( glGetError() != GL_NO_ERROR )
        {
            vtkErrorMacro( "Error setting texture for transfer function for volume " << i  );
            return 0;
        }
    }

    return 1;
}

#include "vtkImageImport.h"
#include "vtkPNGWriter.h"

int vtkPRISMVolumeMapper::UpdateDepthBufferTexture( int width, int height )
{
    // Create texture if it doesn't exist and bind it
    if( this->DepthBufferTextureId == 0 )
    {
        glGenTextures( 1, &this->DepthBufferTextureId );
    }

    glEnable( vtkgl::TEXTURE_RECTANGLE_ARB );
    glBindTexture( vtkgl::TEXTURE_RECTANGLE_ARB, this->DepthBufferTextureId );

    // If size has changed, reallocate texture
    if( width != this->DepthBufferTextureSize[0] || height != this->DepthBufferTextureSize[1] )
    {
        glTexParameteri( vtkgl::TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri( vtkgl::TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf( vtkgl::TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf( vtkgl::TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri( vtkgl::TEXTURE_RECTANGLE_ARB, vtkgl::TEXTURE_COMPARE_MODE_ARB, GL_NONE );
        glTexImage2D( vtkgl::TEXTURE_RECTANGLE_ARB, 0, vtkgl::DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0 );
        this->DepthBufferTextureSize[0] = width;
        this->DepthBufferTextureSize[1] = height;
    }

    // Now copy depth buffer to texture
    glReadBuffer( GL_BACK );
    glCopyTexSubImage2D( vtkgl::TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0, width, height );

    // ===========================================================================
    // TEMP DEBUG : save depth buffer
    /*int bufferSize = width * height;
    int byteSize = bufferSize * sizeof(unsigned short);
    unsigned short * buffer = new unsigned short[ bufferSize ];

    glGetTexImage( vtkgl::TEXTURE_RECTANGLE_ARB, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, buffer );

    vtkImageImport * importer=vtkImageImport::New();
    importer->CopyImportVoidPointer( buffer, static_cast<int>(byteSize) );
    importer->SetDataScalarTypeToUnsignedShort();
    importer->SetNumberOfScalarComponents(1);
    importer->SetWholeExtent(0,width-1,0,height-1,0,0);
    importer->SetDataExtentToWholeExtent();

    importer->Update();

    vtkPNGWriter *writer=vtkPNGWriter::New();
    writer->SetFileName("/home/simon/depth.png");
    writer->SetInputConnection(importer->GetOutputPort());
    importer->Delete();
    writer->Write();
    writer->Delete();

    delete[] buffer;*/
    // TEMP DEBUG
    // ===========================================================================

    glBindTexture( vtkgl::TEXTURE_RECTANGLE_ARB, 0 );
    glDisable( vtkgl::TEXTURE_RECTANGLE_ARB );

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
    vtkMatrix4x4::Multiply4x4( worldToDataset.GetPointer(), datasetToTexture.GetPointer(), worldToTexture.GetPointer() );

    // Compute eye to texture
    vtkMatrix4x4 * worldToEye = renderer->GetActiveCamera()->GetViewTransformMatrix();
    vtkSmartPointer<vtkMatrix4x4> eyeToTexture = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMatrix4x4::Multiply4x4( worldToEye, worldToTexture.GetPointer(), eyeToTexture.GetPointer() );

    // Set the matrix with the shader
    bool res = this->VolumeShader->SetVariable( "eyeToTexture", eyeToTexture );
    return res;
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

//===============================================================
// Need
//      OpenGL 2.0
//      GL_EXT_framebuffer_object
//      GL_ARB_texture_float
//===============================================================
void vtkPRISMVolumeMapper::LoadExtensions( vtkRenderWindow * window )
{
    this->UnsupportedExtensions.clear();
    this->GlExtensionsLoaded = true;
    vtkOpenGLExtensionManager * extensions = vtkOpenGLExtensionManager::New();
    extensions->SetRenderWindow( window );

    if( !extensions->ExtensionSupported("GL_VERSION_2_0") )
    {
        this->GlExtensionsLoaded = false;
        this->UnsupportedExtensions.push_back( std::string(" OpenGL 2.0 required but not supported" ) );
    }

    if( !extensions->ExtensionSupported("GL_EXT_framebuffer_object" ) )
    {
        this->GlExtensionsLoaded = false;
        this->UnsupportedExtensions.push_back( std::string("GL_EXT_framebuffer_object is required but not supported" ) );
    }

    if( !extensions->ExtensionSupported("GL_ARB_texture_float" ) )
    {
        this->GlExtensionsLoaded = false;
        this->UnsupportedExtensions.push_back( std::string("GL_ARB_texture_float is required but not supported" ) );
    }

    // Really load now that we know everything is supported
    if( this->GlExtensionsLoaded )
    {
        extensions->LoadExtension( "GL_VERSION_1_2" );
        extensions->LoadExtension( "GL_VERSION_1_3" );
        extensions->LoadExtension( "GL_VERSION_2_0" );
        extensions->LoadExtension( "GL_EXT_framebuffer_object" );
        extensions->LoadExtension( "GL_ARB_texture_float" );
    }

    extensions->Delete();
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

#include "vtkInformation.h"

int vtkPRISMVolumeMapper::FillInputPortInformation( int port, vtkInformation * info )
{
    if (!this->Superclass::FillInputPortInformation( port, info ) )
    {
        return 0;
    }
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



