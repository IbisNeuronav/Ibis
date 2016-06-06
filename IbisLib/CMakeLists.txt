PROJECT( IGNS_IBISLIB )

#================================
# Define source files
#================================

SET( IGNS_IBISLIB_SRC
                     application.cpp
                     scenemanager.cpp
                     hardwaremodule.cpp
                     filereader.cpp
                     view.cpp
                     sceneobject.cpp
                     imageobject.cpp
                     triplecutplaneobject.cpp
                     worldobject.cpp
                     dtiobject.cpp
                     pickerobject.cpp
                     polydataobject.cpp
                     pointcloudobject.cpp
                     pointsobject.cpp
                     pointrepresentation.cpp
                     surfacecuttingplane.cpp
                     sceneinfo.cpp
                     octants.cpp
                     polydataclipper.cpp
                     volumerenderingobject.cpp
                     serializerhelper.cpp
                     generictrackedobject.cpp
                     usmask.cpp
                     updatemanager.cpp
                     usprobeobject.cpp
                     pointerobject.cpp
                     cameraobject.cpp
                     usacquisitionobject.cpp
                     trackedvideobuffer.cpp
                     toolplugininterface.cpp
                     vtkopencvbridge.cpp
                     lookuptablemanager.cpp
                     linesfactory.cpp
                     simplepropcreator.cpp
                     ibismath.cpp
                     gui/aboutbicigns.cpp
                     gui/hardwaresettings.cpp
                     gui/quadviewwindow.cpp
                     gui/objecttreewidget.cpp
                     gui/objecttreemodel.cpp
                     gui/imageobjectsettingsdialog.cpp
                     gui/imageobjectvolumesettingswidget.cpp
                     gui/polydataobjectsettingsdialog.cpp
                     gui/pointcloudobjectsettingsdialog.cpp
                     gui/dtiobjectsettingswidget.cpp
                     gui/worldobjectsettingswidget.cpp
                     gui/pointpropertieswidget.cpp
                     gui/pointcolorwidget.cpp
                     gui/opendatafiledialog.cpp
                     gui/mincinfowidget.cpp
                     gui/usacquisitionsettingswidget.cpp
                     gui/usmasksettingswidget.cpp
                     gui/zoomfactordialog.cpp
                     gui/transformeditwidget.cpp
                     gui/sceneinfodialog.cpp
                     gui/volumerenderingobjectsettingswidget.cpp
                     gui/volumerenderingsinglevolumesettingswidget.cpp
                     gui/volumeshadereditorwidget.cpp
                     gui/automaticgui.cpp
                     gui/usprobeobjectsettingswidget.cpp
                     gui/pointerobjectsettingsdialog.cpp
                     gui/cameraobjectsettingswidget.cpp
                     gui/generictrackedobjectsettingswidget.cpp
                     gui/triplecutplaneobjectsettingswidget.cpp
                     gui/triplecutplaneobjectmixerwidget.cpp )

SET( IGNS_IBISLIB_HDR
                     dtiobject.h
                     sceneinfo.h
                     trackedvideobuffer.h
                     ibistypes.h
                     serializer.h 
                     serializerhelper.h
                     ignsconfig.h
                     ignsmsg.h
                     toolplugininterface.h
                     objectplugininterface.h
                     vtkopencvbridge.h
                     linesfactory.h
                     simplepropcreator.h
                     ibismath.h
                     gui/guiutilities.h )

SET( IGNS_IBISLIB_HDR_MOC
                         view.h
                         application.h
                         scenemanager.h
                         hardwaremodule.h
                         filereader.h
                         sceneobject.h
                         imageobject.h
                         triplecutplaneobject.h
                         worldobject.h
                         polydataobject.h
                         pointcloudobject.h
                         pointsobject.h
                         pointrepresentation.h
                         surfacecuttingplane.h
                         pickerobject.h
                         sceneinfo.h
                         octants.h
                         polydataclipper.h
                         volumerenderingobject.h
                         generictrackedobject.h
                         usmask.h
                         updatemanager.h
                         usprobeobject.h
                         pointerobject.h
                         cameraobject.h
                         usacquisitionobject.h
                         lookuptablemanager.h
                         gui/aboutbicigns.h
                         gui/hardwaresettings.h
                         gui/quadviewwindow.h
                         gui/objecttreewidget.h
                         gui/objecttreemodel.h
                         gui/imageobjectsettingsdialog.h
                         gui/imageobjectvolumesettingswidget.h
                         gui/polydataobjectsettingsdialog.h
                         gui/pointcloudobjectsettingsdialog.h
                         gui/dtiobjectsettingswidget.h
                         gui/worldobjectsettingswidget.h
                         gui/pointpropertieswidget.h
                         gui/pointcolorwidget.h
                         gui/opendatafiledialog.h
                         gui/mincinfowidget.h
                         gui/usacquisitionsettingswidget.h
                         gui/usmasksettingswidget.h
                         gui/zoomfactordialog.h
                         gui/transformeditwidget.h
                         gui/sceneinfodialog.h
                         gui/volumerenderingobjectsettingswidget.h
                         gui/volumerenderingsinglevolumesettingswidget.h
                         gui/volumeshadereditorwidget.h
                         gui/automaticgui.h
                         gui/pointerobjectsettingsdialog.h
                         gui/cameraobjectsettingswidget.h
                         gui/triplecutplaneobjectsettingswidget.h
                         gui/triplecutplaneobjectmixerwidget.h
                         gui/generictrackedobjectsettingswidget.h
                         gui/usprobeobjectsettingswidget.h )

set( IGNS_IBISLIB_UI
            gui/aboutbicigns.ui
            gui/hardwaresettings.ui
            gui/imageobjectsettingsdialog.ui
            gui/imageobjectvolumesettingswidget.ui
            gui/usacquisitionsettingswidget.ui
            gui/usmasksettingswidget.ui
            gui/mincinfowidget.ui
            gui/objecttreewidget.ui
            gui/opendatafiledialog.ui
            gui/pointpropertieswidget.ui
            gui/pointcolorwidget.ui
            gui/polydataobjectsettingsdialog.ui
            gui/pointcloudobjectsettingsdialog.ui
            gui/dtiobjectsettingswidget.ui
            gui/worldobjectsettingswidget.ui
            gui/zoomfactordialog.ui
            gui/sceneinfodialog.ui
            gui/transformeditwidget.ui
            gui/volumerenderingobjectsettingswidget.ui
            gui/volumerenderingsinglevolumesettingswidget.ui
            gui/volumeshadereditorwidget.ui
            gui/pointerobjectsettingsdialog.ui
            gui/cameraobjectsettingswidget.ui
            gui/triplecutplaneobjectsettingswidget.ui
            gui/triplecutplaneobjectmixerwidget.ui
            gui/generictrackedobjectsettingswidget.ui
            gui/usprobeobjectsettingswidget.ui )


# moc Qt source file without a ui file
qt5_wrap_cpp( IGNS_IBISLIB_MOC ${IGNS_IBISLIB_HDR_MOC} )

# Generate code for .ui files -> ui_xxx.h
qt5_wrap_ui( IGNS_IBISLIB_HDR_UI ${IGNS_IBISLIB_UI} )

# make rules to create c++ source from Qt resource files
set( IGNS_IBISLIB_RCC IbisLib.qrc )
qt5_add_resources( IGNS_IBISLIB_RCC_SRC ${IGNS_IBISLIB_RCC})

#================================
# Includes dir for vtkIbis libs
#================================
include_directories( ${IGNS_VTK_QT_INCLUDE_DIR} )
include_directories( ${VTK_TRACKER_INCLUDE_DIR} )
include_directories( ${VTK_EXTENSIONS_INCLUDE_DIR} )
include_directories( ${VTK_VIDEO_INCLUDE_DIR} )
include_directories( ${VTK_ULTRASOUND_INCLUDE_DIR} )
include_directories( ${VTK_MNI_INCLUDE_DIR} )
include_directories( ${VTK_MINC_INCLUDE_DIR} )
include_directories( ${IGNS_VTKIBIS_VOLUME_RENDERING_INCLUDE_DIR} )
include_directories( ${IGNS_VTK_INCLUDE_DIR} )

#================================
# Define output
#================================
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} )           # so that non-generated gui files can include main files
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/gui )       # for non-generated gui .h files
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} )           # for files generated by Qt
set( IGNS_IBISLIB_all_src ${IGNS_IBISLIB_SRC} ${IGNS_IBISLIB_MOC} ${IGNS_IBISLIB_HDR} ${IGNS_IBISLIB_HDR_MOC} ${IGNS_IBISLIB_HDR_UI} ${IGNS_IBISLIB_RCC_SRC} )
ADD_LIBRARY( IbisLib ${IGNS_IBISLIB_all_src})

IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  SET_TARGET_PROPERTIES( IbisLib PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )

#================================
# Define include dir for
# dependent projects
#================================
SET( IGNS_IBISLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/gui CACHE PATH "Include directory for IbisLib header files" )
MARK_AS_ADVANCED( IGNS_IBISLIB_INCLUDE_DIR )
