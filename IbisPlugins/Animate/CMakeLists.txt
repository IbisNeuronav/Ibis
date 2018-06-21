# define sources
set( PluginSrc animateplugininterface.cpp animatewidget.cpp timelinewidget.cpp
                DomeRenderer.cpp vtkOffscreenCamera.cxx CameraAnimation.cpp
                CameraKey.cpp transferfunctionkey.cpp doublevalueanimation.cpp offscreenrenderer.cpp
                KeyframeEditorWidget.cpp )

set( PluginHdr DomeRenderer.h vtkOffscreenCamera.h Animation.h
            CameraAnimation.h CameraKey.h transferfunctionkey.h doublevalueanimation.h offscreenrenderer.h )

set( PluginHdrMoc animatewidget.h animateplugininterface.h  timelinewidget.h KeyframeEditorWidget.h )

set( PluginUi animatewidget.ui timelinewidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

#add library specific to this plugin
target_link_libraries( ${PluginName} svl PRISMVolumeRender )
