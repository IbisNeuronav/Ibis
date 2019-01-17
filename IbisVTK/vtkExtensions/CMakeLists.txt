#================================
# Define sources
#================================
SET( VTK_EXTENSIONS_SRC
    vtkIbisImagePlaneMapper.cxx
    vtkNShapeCalibrationWidget.cxx
    vtkCircleWithCrossSource.cxx
    vtkMultiInteractorObserver.cxx
    vtkMulti3DWidget.cxx
    vtkMultiImagePlaneWidget.cxx
    vtkMultiTextureMapToPlane.cxx
    vtkPiecewiseFunctionLookupTable.cxx
    vtkInteractorStyleImage2.cxx
    vtkOBJReader2.cxx
    vtkSimpleMapper3D.cxx
    vtkSimpleProp3D.cxx
    stringtools.cpp
    vtkMatrix4x4Operators.cxx
    GlslShader.cpp
    DrawableTexture.cpp )

SET( VTK_EXTENSIONS_HDR
    vtkIbisImagePlaneMapper.h
    vtkNShapeCalibrationWidget.h
    vtkCircleWithCrossSource.h
    vtkMultiInteractorObserver.h
    vtkMulti3DWidget.h
    vtkMultiImagePlaneWidget.h
    vtkMultiTextureMapToPlane.h
    vtkPiecewiseFunctionLookupTable.h
    vtkInteractorStyleImage2.h
    vtkOBJReader2.h
    vtkSimpleMapper3D.h
    vtkSimpleProp3D.h
    vtkGenericParam.h
    stringtools.h
    vtkMatrix4x4Operators.h
    GlslShader.h
    DrawableTexture.h )

#================================
# Process GLSL shader files
#================================
set( LibGlsl vtkIbisImagePlaneMapper_FS.glsl )
set( LibGlslSrc )
foreach (shader_file IN LISTS LibGlsl)
  vtk_encode_string(
    INPUT         "${shader_file}"
    HEADER_OUTPUT shader_h
    SOURCE_OUTPUT shader_cxx)
  list(APPEND LibGlslSrc ${shader_file} ${shader_h} ${shader_cxx})
endforeach ()

#================================
# Define output
#================================
IF (BUILD_SHARED_LIBS)
ADD_LIBRARY( vtkExtensions SHARED ${VTK_EXTENSIONS_SRC} ${VTK_EXTENSIONS_HDR} )
ELSE( BUILD_SHARED_LIBS )
ADD_LIBRARY( vtkExtensions ${VTK_EXTENSIONS_SRC} ${VTK_EXTENSIONS_HDR} ${LibGlslSrc} )
ENDIF( BUILD_SHARED_LIBS )

target_include_directories( vtkExtensions  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )
