SET( ULTRASOUND_SRC vtkTrackedVideoBuffer.cxx )
SET( ULTRASOUND_HDR vtkTrackedVideoBuffer.h )

#================================
# Define sources
#================================
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/../vtkVideo )  # for vtkVideo
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/../vtkTracker )  # for vtkTracker

ADD_LIBRARY( vtkUltrasound ${ULTRASOUND_SRC} ${ULTRASOUND_HDR} )
target_link_libraries( vtkUltrasound ibisVTKHeaders )

if( IBIS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkUltrasound)
    INSTALL_FILES(/include FILES ${ULTRASOUND_HDR})
endif( IBIS_INSTALL_LIBS_AND_HEADERS )
