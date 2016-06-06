PROJECT( IGNS_VTK_MINC )

#================================
# Define sources
#================================
SET( IGNS_VTK_MINC_SRC
    vtkMINCWriter.cxx
    vtkMINCReader.cxx
    vtkMINCImageAttributes2.cxx
    vtkSimpleMINCReader.cxx
    vtkMINCIcv.cxx
    vtkMNIOBJReader.cxx )

SET( IGNS_VTK_MINC_HDR
    vtkMINCWriter.h
    vtkMINCReader.h
    vtkMINCImageAttributes2.h
    vtkSimpleMINCReader.h
    vtkMINC2.h
    vtkMINCIcv.h
    vtkMNIOBJReader.h )

#================================
# Define output
#================================
ADD_LIBRARY( vtkMinc ${IGNS_VTK_MINC_SRC} ${IGNS_VTK_MINC_HDR} )
LINK_DIRECTORIES(${MINC4ITK_LIBRARY_DIRS})
TARGET_LINK_LIBRARIES( vtkMinc ${MINC4ITK_LIBRARIES} ${MINC2_LIBRARIES})

if( IGNS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkMinc)
endif( IGNS_INSTALL_LIBS_AND_HEADERS )

#================================
# Define include dir for
# dependent projects
#================================
SET( VTK_MINC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Include directory for vtkMinc .h files" )
MARK_AS_ADVANCED( VTK_MINC_INCLUDE_DIR )
