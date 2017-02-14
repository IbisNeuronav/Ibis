SET( ULTRASOUND_SRC vtkTrackedVideoBuffer.cxx )
SET( ULTRASOUND_HDR vtkTrackedVideoBuffer.h )

#================================
# Define sources
#================================
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/../vtkVideo )  # for vtkVideo
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/../vtkTracker )  # for vtkTracker
INCLUDE_DIRECTORIES( ${IBIS_VTK_INCLUDE_DIR} )  # for vtkExtensions

ADD_LIBRARY( vtkUltrasound ${ULTRASOUND_SRC} ${ULTRASOUND_HDR} )

if( IBIS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkUltrasound)
    INSTALL_FILES(/include FILES ${ULTRASOUND_HDR})
endif( IBIS_INSTALL_LIBS_AND_HEADERS )

#================================
# Define include dir for
# dependent projects
#================================
SET( VTK_ULTRASOUND_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE )

