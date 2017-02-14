#================================
# Define sources
#================================
SET( VTK_MNI_SRC
    vtkMNIOBJReader.cxx
    vtkTagReader.cxx
    vtkTagWriter.cxx 
    vtkXFMReader.cxx
    vtkXFMWriter.cxx )

SET( VTK_MNI_HDR
    vtkMNIOBJReader.h
    vtkTagReader.h
    vtkTagWriter.h
    vtkXFMReader.h
    vtkXFMWriter.h )

include_directories( ${IBIS_VTK_INCLUDE_DIR} )

#================================
# Define output
#================================
ADD_LIBRARY( vtkMNI ${VTK_MNI_SRC} ${VTK_MNI_HDR} )

if( IBIS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkMNI)
endif( IBIS_INSTALL_LIBS_AND_HEADERS )
