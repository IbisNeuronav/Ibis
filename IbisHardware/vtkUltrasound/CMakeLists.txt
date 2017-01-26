PROJECT( IGNS_VTK_ULTRASOUND )

SET( IGNS_VTK_ULTRASOUND_SRC vtkTrackedVideoBuffer.cxx )
SET( IGNS_VTK_ULTRASOUND_HDR vtkTrackedVideoBuffer.h )

#================================
# Define sources
#================================
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/../vtkVideo )  # for vtkVideo
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/../vtkTracker )  # for vtkTracker
INCLUDE_DIRECTORIES( ${VTK_EXTENSIONS_INCLUDE_DIR} )  # for vtkExtensions
#INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/../vtkExtensions )  # for vtkExtensions

ADD_LIBRARY( vtkUltrasound ${IGNS_VTK_ULTRASOUND_SRC} ${IGNS_VTK_ULTRASOUND_HDR} )

if( IGNS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkUltrasound)
    INSTALL_FILES(/include FILES ${IGNS_VTK_ULTRASOUND_HDR})
endif( IGNS_INSTALL_LIBS_AND_HEADERS )

#================================
# Define include dir for
# dependent projects
#================================
SET( VTK_ULTRASOUND_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Include directory for vtkUltrasound .h files" )
MARK_AS_ADVANCED( VTK_ULTRASOUND_INCLUDE_DIR )

