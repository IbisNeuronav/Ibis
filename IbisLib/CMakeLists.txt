PROJECT( IBISLIB )

#================================
# Define source files
#================================

SET( IBISLIB_SRC
                     ibisapi.cpp
                     application.cpp
                     mainwindow.cpp
                     scenemanager.cpp
                     filereader.cpp
                     view.cpp
                     sceneobject.cpp
                     trackedsceneobject.cpp
                     imageobject.cpp
                     triplecutplaneobject.cpp
                     worldobject.cpp
                     polydataobject.cpp
                     pointcloudobject.cpp
                     pointsobject.cpp
                     pointrepresentation.cpp
                     serializerhelper.cpp
                     usmask.cpp
                     updatemanager.cpp
                     usprobeobject.cpp
                     pointerobject.cpp
                     cameraobject.cpp
                     usacquisitionobject.cpp
                     trackedvideobuffer.cpp
                     toolplugininterface.cpp
                     lookuptablemanager.cpp
                     linesfactory.cpp
                     simplepropcreator.cpp
                     ibismath.cpp
                     ibisplugin.cpp
                     ibisitkvtkconverter.cpp
                     gui/aboutbicigns.cpp
                     gui/aboutpluginswidget.cpp
                     gui/trackerstatusdialog.cpp
                     gui/quadviewwindow.cpp
                     gui/objecttreewidget.cpp
                     gui/objecttreemodel.cpp
                     gui/imageobjectsettingsdialog.cpp
                     gui/imageobjectvolumesettingswidget.cpp
                     gui/polydataobjectsettingsdialog.cpp
                     gui/pointcloudobjectsettingsdialog.cpp
                     gui/worldobjectsettingswidget.cpp
                     gui/pointpropertieswidget.cpp
                     gui/pointcolorwidget.cpp
                     gui/opendatafiledialog.cpp
                     gui/mincinfowidget.cpp
                     gui/usacquisitionsettingswidget.cpp
                     gui/exportacquisitiondialog.cpp
                     gui/usmasksettingswidget.cpp
                     gui/transformeditwidget.cpp
                     gui/automaticgui.cpp
                     gui/usprobeobjectsettingswidget.cpp
                     gui/pointerobjectsettingsdialog.cpp
                     gui/cameraobjectsettingswidget.cpp
                     gui/triplecutplaneobjectsettingswidget.cpp
                     gui/triplecutplaneobjectmixerwidget.cpp
                     gui/pointercalibrationdialog.cpp )

SET( IBISLIB_HDR
                     trackedvideobuffer.h
                     ibistypes.h
                     serializer.h 
                     serializerhelper.h
                     ibisconfig.h
                     toolplugininterface.h
                     objectplugininterface.h
                     globalobjectplugininterface.h
                     generatorplugininterface.h
                     linesfactory.h
                     simplepropcreator.h
                     ibismath.h
                     ibisitkvtkconverter.h
                     gui/guiutilities.h )

SET( IBISLIB_HDR_MOC
                         ibisapi.h
                         view.h
                         application.h
                         mainwindow.h
                         scenemanager.h
                         ibisplugin.h
                         filereader.h
                         sceneobject.h
                         trackedsceneobject.h
                         imageobject.h
                         triplecutplaneobject.h
                         worldobject.h
                         polydataobject.h
                         pointcloudobject.h
                         pointsobject.h
                         pointrepresentation.h
                         usmask.h
                         updatemanager.h
                         usprobeobject.h
                         pointerobject.h
                         cameraobject.h
                         usacquisitionobject.h
                         gui/aboutbicigns.h
                         gui/aboutpluginswidget.h
                         gui/trackerstatusdialog.h
                         gui/quadviewwindow.h
                         gui/objecttreewidget.h
                         gui/objecttreemodel.h
                         gui/imageobjectsettingsdialog.h
                         gui/imageobjectvolumesettingswidget.h
                         gui/polydataobjectsettingsdialog.h
                         gui/pointcloudobjectsettingsdialog.h
                         gui/worldobjectsettingswidget.h
                         gui/pointpropertieswidget.h
                         gui/pointcolorwidget.h
                         gui/opendatafiledialog.h
                         gui/mincinfowidget.h
                         gui/usacquisitionsettingswidget.h
                         gui/exportacquisitiondialog.h
                         gui/usmasksettingswidget.h
                         gui/transformeditwidget.h
                         gui/automaticgui.h
                         gui/pointerobjectsettingsdialog.h
                         gui/cameraobjectsettingswidget.h
                         gui/triplecutplaneobjectsettingswidget.h
                         gui/triplecutplaneobjectmixerwidget.h
                         gui/usprobeobjectsettingswidget.h
                         gui/pointercalibrationdialog.h )

set( IBISLIB_UI
            gui/aboutbicigns.ui
            gui/aboutpluginswidget.ui
            gui/imageobjectsettingsdialog.ui
            gui/imageobjectvolumesettingswidget.ui
            gui/usacquisitionsettingswidget.ui
            gui/exportacquisitiondialog.ui
            gui/usmasksettingswidget.ui
            gui/objecttreewidget.ui
            gui/opendatafiledialog.ui
            gui/pointpropertieswidget.ui
            gui/pointcolorwidget.ui
            gui/polydataobjectsettingsdialog.ui
            gui/pointcloudobjectsettingsdialog.ui
            gui/worldobjectsettingswidget.ui
            gui/transformeditwidget.ui
            gui/pointerobjectsettingsdialog.ui
            gui/cameraobjectsettingswidget.ui
            gui/triplecutplaneobjectsettingswidget.ui
            gui/triplecutplaneobjectmixerwidget.ui
            gui/usprobeobjectsettingswidget.ui
            gui/pointercalibrationdialog.ui )


# moc Qt source file without a ui file
qt5_wrap_cpp( IBISLIB_MOC ${IBISLIB_HDR_MOC} )

# Generate code for .ui files -> ui_xxx.h
qt5_wrap_ui( IBISLIB_HDR_UI ${IBISLIB_UI} )

# make rules to create c++ source from Qt resource files
set( IBISLIB_RCC IbisLib.qrc )
qt5_add_resources( IBISLIB_RCC_SRC ${IBISLIB_RCC})

#================================
# Define output
# ibis_BINARY_DIR is automatically computed by CMake using the main project name ibis
# this directory contains version.h and githash.h needed by application.cpp
#================================
set( IBISLIB_all_src ${IBISLIB_SRC} ${IBISLIB_MOC} ${IBISLIB_HDR} ${IBISLIB_HDR_MOC} ${IBISLIB_HDR_UI} ${IBISLIB_RCC_SRC} )
ADD_LIBRARY( IbisLib ${IBISLIB_all_src})

target_link_libraries( IbisLib svl vtkExtensions)
target_include_directories( IbisLib PUBLIC  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/gui
    PRIVATE ${ibis_BINARY_DIR} )
IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  SET_TARGET_PROPERTIES( IbisLib PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )

#================================
# Define include dir for
# dependent projects
#================================
SET( IBISLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/gui PARENT_SCOPE )

set (UI_FILES "ui*")
set (MOC_FILES "moc*")
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${UI_FILES})
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${MOC_FILES})
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/gui/${MOC_FILES})

#set (PARAM_FILES "moc_*.cpp_parameters")
#set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${PARAM_FILES})
#set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/gui/${PARAM_FILES})

