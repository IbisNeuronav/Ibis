PROJECT( IGNS_VTKIBIS_VOLUME_RENDERING )

#================================
# Define sources
#================================
set( LibSrc vtkIbisGLSLVolumeRaycastMapper.cxx vtkIbisImagePlaneMapper.cxx GlslShader.cpp DrawableTexture.cpp )
set( LibHdr vtkIbisGLSLVolumeRaycastMapper.h vtkIbisImagePlaneMapper.h GlslShader.h DrawableTexture.h )
set( LibGlsl vtkIbisGLSLVolumeRaycast_FS )

#================================
# Create custom commands to
# encode each glsl file into a
# C string literal in a header
# file.
#================================

foreach( file ${LibGlsl} )
  set( shader_src ${CMAKE_CURRENT_SOURCE_DIR}/${file}.glsl )
  set( shader_cxx ${CMAKE_CURRENT_BINARY_DIR}/${file}.cxx )
  set( shader_h ${CMAKE_CURRENT_BINARY_DIR}/${file}.h )
  add_custom_command(
    OUTPUT ${shader_cxx} ${shader_h}
    DEPENDS ${shader_src}
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${shader_cxx} ${shader_src} ${file} --build-header " " vtkSystemIncludes.h )
  set( LibGlslSrc ${LibGlslSrc} ${shader_src} ${shader_h} ${shader_cxx} )
endforeach(file)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${VTK_EXTENSIONS_INCLUDE_DIR} )

#================================
# Define output
#================================
add_library( vtkIbisVolumeRendering ${LibSrc} ${LibHdr} ${LibGlslSrc} )

# install
if( IGNS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkIbisVolumeRendering )
    INSTALL_FILES(/include FILES ${LibHdr})
endif()

#================================
# Define include dir for
# dependent projects
#================================
SET( IGNS_VTKIBIS_VOLUME_RENDERING_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Include directory for vtkIbisVolumeRendering .h files" )
MARK_AS_ADVANCED( IGNS_VTKIBIS_VOLUME_RENDERING_INCLUDE_DIR )

