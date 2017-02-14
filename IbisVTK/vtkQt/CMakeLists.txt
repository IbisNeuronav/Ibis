SET( IBIS_VTK_QT_SRC
           vtkQtImageViewer.cpp
           vtkQtMatrixDialog.cpp
           vtkQtHistogramWidget.cpp
           vtkQtPiecewiseFunctionWidget.cpp
           vtkQtColorTransferFunctionWidget.cpp
           vtkqtrenderwindow.cpp )

SET( IBIS_VTK_QT_HDR_MOC vtkQtImageViewer.h vtkQtMatrixDialog.h vtkqtrenderwindow.h )
set( IBIS_VTK_QT_PLUGIN_HDR_MOC vtkQtHistogramWidget.h vtkQtPiecewiseFunctionWidget.h vtkQtColorTransferFunctionWidget.h )

if(APPLE)
    set( IBIS_VTK_QT_SRC ${IBIS_VTK_QT_SRC} retinadisplayhack.mm retinadisplayhack.h )
endif()

# moc Qt source file without a ui file
qt5_wrap_cpp( IBIS_VTK_QT_MOC ${IBIS_VTK_QT_HDR_MOC} )
qt5_wrap_cpp( IBIS_VTK_QT_PLUGIN_MOC ${IBIS_VTK_QT_PLUGIN_HDR_MOC} )

INCLUDE_DIRECTORIES(${QT_EXTENSIONS_INCLUDE_DIR})

#================================
# Define sources
#================================
INCLUDE_DIRECTORIES(${IBIS_VTK_INCLUDE_DIR})

set( vtkQt_all_src ${IBIS_VTK_QT_SRC} ${IBIS_VTK_QT_HDR} ${IBIS_VTK_QT_HDR_MOC} ${IBIS_VTK_QT_MOC} ${IBIS_VTK_QT_PLUGING_HDR_MOC} ${IBIS_VTK_QT_PLUGIN_MOC} )
ADD_LIBRARY( vtkQt  ${vtkQt_all_src} )

IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  SET_TARGET_PROPERTIES( vtkQt PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )

if( IBIS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib vtkQt)
    INSTALL_FILES(/include FILES ${IBIS_VTK_QT_HDR} )
endif( IBIS_INSTALL_LIBS_AND_HEADERS )

#================================
# Build plugin that allows
# QtDesigner and QtCreator to
# show some of our custom widgets
# in the IDE.
#================================
add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_NO_DEBUG)
add_definitions(-DQT_SHARED)
add_definitions(-DQDESIGNER_EXPORT_WIDGETS)
add_definitions(-DQT_SCRIPT_LIB)
add_definitions(-DQT_XML_LIB)
add_definitions(-DQT_GUI_LIB)
add_definitions(-DQT_CORE_LIB)
include_directories(${QT_QTDESIGNER_INCLUDE_DIR})

set( vtkQtPluginSrc vtkQtPlugin.cpp vtkQtHistogramWidget.cpp vtkQtPiecewiseFunctionWidget.cpp vtkQtColorTransferFunctionWidget.cpp )
set( vtkQtPluginHdrMoc vtkQtPlugin.h )
qt5_wrap_cpp( vtkQtPluginSrcMoc ${vtkQtPluginHdrMoc} )

# add vtkQtPlugin from sources
#ADD_LIBRARY( vtkQtPlugin SHARED ${vtkQtPluginSrc} ${vtkQtPluginSrcMoc} ${vtkQtPluginHdrMoc} ${IBIS_VTK_QT_PLUGING_HDR_MOC} ${IBIS_VTK_QT_PLUGIN_MOC} )

# link with release version of Qt libs

#TARGET_LINK_LIBRARIES( vtkQtPlugin ${QT_LIBRARIES} vtkFiltering )
#TARGET_LINK_LIBRARIES( vtkQtPlugin ${QT_LIBRARIES})
