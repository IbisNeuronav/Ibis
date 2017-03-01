# This plugin requires OpenCV. Not the case for general plugins
if( NOT OpenCV_FOUND )
    message( FATAL_ERROR "CameraCalibration plugin requires OpenCV. Set OpenCV dir or disable build of CameraCalibration plugin." )
endif()

include_directories( ${LandmarkRegistrationIncludeDir} ${IBIS_VTK_INCLUDE_DIR} ${IBIS_VTKOPENCV_INCLUDE_DIR} )

# define sources
set( PluginSrc cameracalibrationplugininterface.cpp cameracalibrationwidget.cpp cameracalibrationsidepanelwidget.cpp cameracalibrator.cpp )
set( PluginHdr cameracalibrator.h )
set( PluginHdrMoc cameracalibrationwidget.h cameracalibrationplugininterface.h cameracalibrationsidepanelwidget.h )
set( PluginUi cameracalibrationwidget.ui cameracalibrationsidepanelwidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

#add library specific to this plugin
target_link_libraries( ${PluginName} ibisVTKOpenCVBridge )

# Command-line exec to test the calibration
#add_executable( CalibrationXValidation CalibrationXValidation.cpp )
#target_link_libraries( CalibrationXValidation IbisLib CameraCalibration ${OpenCV_LIBS} vtkCommon vtkHybrid vtkWidgets QVTK ${QT_LIBRARIES} )
