SET( ULTRASOUND_SRC vtkTrackedVideoBuffer.cxx )
SET( ULTRASOUND_HDR vtkTrackedVideoBuffer.h )

#================================
# Define sources
#================================
ADD_LIBRARY( vtkUltrasound ${ULTRASOUND_SRC} ${ULTRASOUND_HDR} )
target_link_libraries( vtkUltrasound ibisVTKHeaders IbisHardwareHeaders )

if( IBIS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkUltrasound)
    INSTALL_FILES(/include FILES ${ULTRASOUND_HDR})
endif( IBIS_INSTALL_LIBS_AND_HEADERS )
