# define sources
set(  PluginSrc
      pediclescrewnavigationplugininterface.cpp
      vertebraregistrationwidget.cpp
      screwnavigationwidget.cpp
      screwtablewidget.cpp
      screwproperties.cpp
      secondaryusacquisition.cpp
      )

set( PluginHdrMoc 
     vertebraregistrationwidget.h
     pediclescrewnavigationplugininterface.h
     screwnavigationwidget.h
     screwtablewidget.h
     screwproperties.h
     secondaryusacquisition.h
    )

set( PluginUi vertebraregistrationwidget.ui screwnavigationwidget.ui screwtablewidget.ui )

IF (NOT IBIS_PLUGINS_BUILD_GPU_RigidRegistration)
   message( SEND_ERROR "Cannot find the GPU_RigidRegistration plugin. PedicleScrewNavigation Plugin depends on GPU_RigidRegistration.\n" )
ENDIF()

IF (NOT IBIS_PLUGINS_BUILD_GPU_VolumeReconstruction)
   message( SEND_ERROR "Cannot find the GPU_VolumeReconstruction plugin. PedicleScrewNavigation Plugin depends on GPU_VolumeReconstruction.\n" )
ENDIF()

add_subdirectory( itkWeightRegistration )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}")

#add library specific to this plugin
target_link_libraries( ${PluginName} GPU_VolumeReconstruction GPU_RigidRegistration itkWeightRegistration )
