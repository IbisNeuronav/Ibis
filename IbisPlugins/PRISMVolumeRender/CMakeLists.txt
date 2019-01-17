#================================
# Create custom commands to
# encode each glsl file into a
# C string literal in a header
# file.
#================================
set( LibGlsl vtkPRISMVolumeRaycast_FS.glsl
             vtkPRISMVolumeRaycast_VS.glsl
             vtkPRISMVolumeMapperBackface_FS.glsl
             vtkPRISMVolumeMapperBackface_VS.glsl )
             
set( LibGlslSrc )
foreach (shader_file IN LISTS LibGlsl)
  vtk_encode_string(
    INPUT         "${shader_file}"
    HEADER_OUTPUT shader_h
    SOURCE_OUTPUT shader_cxx)
  list(APPEND LibGlslSrc ${shader_file} ${shader_h} ${shader_cxx})
endforeach ()

# define sources
set( PluginSrc  PRISMVolumeRenderPluginInterface.cpp
                volumerenderingobject.cpp
                volumerenderingobjectsettingswidget.cpp
                volumerenderingsinglevolumesettingswidget.cpp
                volumeshadereditorwidget.cpp
                vtkPRISMVolumeMapper.cxx
                vtkColoredCube.cxx
                vtkColorPolyData.cxx
                shaderio.cpp
                ${LibGlslSrc} )

set( PluginHdr vtkPRISMVolumeMapper.h
                vtkColoredCube.h
                vtkColorPolyData.h
                shaderio.h
                shadercontrib.h )

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
