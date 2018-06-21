SET( IBIS_VTK_QT_SRC
           vtkQtImageViewer.cpp
           vtkQtMatrixDialog.cpp
           vtkQtHistogramWidget.cpp
           vtkQtPiecewiseFunctionWidget.cpp
           vtkQtColorTransferFunctionWidget.cpp
           vtkqtrenderwindow.cpp )

SET( IBIS_VTK_QT_HDR_MOC vtkQtImageViewer.h vtkQtMatrixDialog.h vtkqtrenderwindow.h )

if(APPLE)
    set( IBIS_VTK_QT_SRC ${IBIS_VTK_QT_SRC} retinadisplayhack.mm retinadisplayhack.h )
endif()

# moc Qt source file without a ui file
qt5_wrap_cpp( IBIS_VTK_QT_MOC ${IBIS_VTK_QT_HDR_MOC} )
qt5_wrap_cpp( IBIS_VTK_QT_PLUGIN_MOC ${IBIS_VTK_QT_PLUGIN_HDR_MOC} )

#================================
# Define sources
#================================

set( vtkQt_all_src ${IBIS_VTK_QT_SRC} ${IBIS_VTK_QT_HDR} ${IBIS_VTK_QT_HDR_MOC} ${IBIS_VTK_QT_MOC} ${IBIS_VTK_QT_PLUGING_HDR_MOC} ${IBIS_VTK_QT_PLUGIN_MOC} )
ADD_LIBRARY( vtkQt  ${vtkQt_all_src} )

target_link_libraries( vtkQt vtkExtensions )

IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  SET_TARGET_PROPERTIES( vtkQt PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )

if( IBIS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkQt)
    INSTALL_FILES(/include FILES ${IBIS_VTK_QT_HDR} )
endif( IBIS_INSTALL_LIBS_AND_HEADERS )
