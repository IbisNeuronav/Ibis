PROJECT( IGNS_VTK_MNI )

#================================
# Define sources
#================================
SET( IGNS_VTK_MNI_SRC
    vtkMNIOBJReader.cxx
    vtkTagReader.cxx
    vtkTagWriter.cxx 
    vtkXFMReader.cxx
    vtkXFMWriter.cxx )

SET( IGNS_VTK_MNI_HDR
    vtkMNIOBJReader.h
    vtkTagReader.h
    vtkTagWriter.h
    vtkXFMReader.h
    vtkXFMWriter.h )

include_directories( ${VTK_EXTENSIONS_INCLUDE_DIR} )
include_directories( ${IGNS_VTK_INCLUDE_DIR} )

#================================
# Define output
#================================
ADD_LIBRARY( vtkMNI ${IGNS_VTK_MNI_SRC} ${IGNS_VTK_MNI_HDR} )

if( IGNS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkMNI)
endif( IGNS_INSTALL_LIBS_AND_HEADERS )

#================================
# Define include dir for
# dependent projects
#================================
SET( VTK_MNI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Include directory for vtkMNI .h files" )
MARK_AS_ADVANCED( VTK_MNI_INCLUDE_DIR )
