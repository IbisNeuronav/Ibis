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
   message( WARNING "PedicleScrewNavigation Plugin depends on GPU_RigidRegistration, enabling it.\n" )
   set( IBIS_PLUGINS_BUILD_GPU_RigidRegistration ON )
ENDIF()

IF (NOT IBIS_PLUGINS_BUILD_GPU_VolumeReconstruction)
   message( WARNING "PedicleScrewNavigation Plugin depends on GPU_VolumeReconstruction. enabling it.\n" )
   set( IBIS_PLUGINS_BUILD_GPU_VolumeReconstruction ON )
ENDIF()

add_subdirectory( itkWeightRegistration )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

#add library specific to this plugin
target_link_libraries( ${PluginName} GPU_VolumeReconstruction GPU_RigidRegistration itkWeightRegistration )
