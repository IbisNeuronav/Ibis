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

include_directories( ${IBIS_VTK_INCLUDE_DIR} )

#================================
# Define output
#================================
ADD_LIBRARY( vtkMNI ${IGNS_VTK_MNI_SRC} ${IGNS_VTK_MNI_HDR} )

if( IGNS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkMNI)
endif( IGNS_INSTALL_LIBS_AND_HEADERS )
