SET( ULTRASOUND_SRC vtkTrackedVideoBuffer.cxx )
SET( ULTRASOUND_HDR vtkTrackedVideoBuffer.h )

ADD_LIBRARY( vtkUltrasound ${ULTRASOUND_SRC} ${ULTRASOUND_HDR} )
target_link_libraries( vtkUltrasound vtkExtensions vtkVideo vtkTracker )
target_include_directories( vtkUltrasound PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )

if( IBIS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkUltrasound)
    INSTALL_FILES(/include FILES ${ULTRASOUND_HDR})
endif( IBIS_INSTALL_LIBS_AND_HEADERS )
