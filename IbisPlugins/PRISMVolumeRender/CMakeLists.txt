project( ${PluginName} )

#================================
# Create custom commands to
# encode each glsl file into a
# C string literal in a header
# file.
#================================
set( LibGlsl vtkPRISMVolumeRaycast_FS )

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

# define sources
set( PluginSrc  PRISMVolumeRenderPluginInterface.cpp
                volumerenderingobject.cpp
                volumerenderingobjectsettingswidget.cpp
                volumerenderingsinglevolumesettingswidget.cpp
                volumeshadereditorwidget.cpp
                vtkPRISMVolumeMapper.cxx
                ${LibGlslSrc} )

set( PluginHdr vtkPRISMVolumeMapper.h )

set( PluginHdrMoc   PRISMVolumeRenderPluginInterface.h
                    volumerenderingobject.h
                    volumerenderingobjectsettingswidget.h
                    volumerenderingsinglevolumesettingswidget.h
                    volumeshadereditorwidget.h )

set( PluginUi   volumerenderingobjectsettingswidget.ui
                volumerenderingsinglevolumesettingswidget.ui
                volumeshadereditorwidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
