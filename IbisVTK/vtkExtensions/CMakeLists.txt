PROJECT( IGNS_VTK_EXTENSIONS )

#================================
# Define sources
#================================
SET( IGNS_VTK_EXTENSIONS_SRC
    vtkIbisImagePlaneMapper.cxx
    vtkNShapeCalibrationWidget.cxx
    vtkCircleWithCrossSource.cxx
    vtkMultiInteractorObserver.cxx
    vtkMulti3DWidget.cxx
    vtkMultiImagePlaneWidget.cxx
    vtkMultiTextureMapToPlane.cxx
    vtkPiecewiseFunctionLookupTable.cxx
    vtkInteractorStyleImage2.cxx
    vtkOBJReader2.cxx
    vtkSimpleMapper3D.cxx
    vtkSimpleProp3D.cxx
    stringtools.cpp
    vtkMatrix4x4Operators.cxx
    GlslShader.cpp
    DrawableTexture.cpp )

SET( IGNS_VTK_EXTENSIONS_HDR
    vtkIbisImagePlaneMapper.h
    vtkNShapeCalibrationWidget.h
    vtkCircleWithCrossSource.h
    vtkMultiInteractorObserver.h
    vtkMulti3DWidget.h
    vtkMultiImagePlaneWidget.h
    vtkMultiTextureMapToPlane.h
    vtkPiecewiseFunctionLookupTable.h
    vtkInteractorStyleImage2.h
    vtkOBJReader2.h 
    vtkSimpleMapper3D.h
    vtkSimpleProp3D.h
    vtkGenericParam.h
    stringtools.h
    vtkMatrix4x4Operators.h
    GlslShader.h
    DrawableTexture.h )

include_directories( ${IGNS_VTK_INCLUDE_DIR} )            # This is for Igns/vtk general include files

#================================
# Define output
#================================
IF (BUILD_SHARED_LIBS)
ADD_LIBRARY( vtkExtensions SHARED ${IGNS_VTK_EXTENSIONS_SRC} ${IGNS_VTK_EXTENSIONS_HDR} )
ELSE( BUILD_SHARED_LIBS )
ADD_LIBRARY( vtkExtensions ${IGNS_VTK_EXTENSIONS_SRC} ${IGNS_VTK_EXTENSIONS_HDR} )
ENDIF( BUILD_SHARED_LIBS )

# install
if( IGNS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkExtensions)
    INSTALL_FILES(/include FILES ${IGNS_VTK_EXTENSIONS_HDR} ${CMAKE_CURRENT_SOURCE_DIR}/../vtkIgnsEvents.h)
endif( IGNS_INSTALL_LIBS_AND_HEADERS )

#================================
# Define include dir for
# dependent projects
#================================
SET( VTK_EXTENSIONS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Include directory for vtkExtensions .h files" )
MARK_AS_ADVANCED( VTK_EXTENSIONS_INCLUDE_DIR )

