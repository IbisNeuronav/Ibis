PROJECT ( IBIS )

# define sources
SET( IBIS_SRC main.cpp commandlinearguments.cpp )
SET( IBIS_HDR commandlinearguments.h )
set( exec_name ibis )

# option to turn on vtk warnings
OPTION( IBIS_ENABLE_VTK_WARNINGS "Enable vtk warnings in the terminal" OFF )
if( NOT IBIS_ENABLE_VTK_WARNINGS )
    add_definitions (-DVTK_NO_WARNINGS )
endif()

# moc Qt source file without a ui file
qt5_wrap_cpp( IBIS_MOC ${IBIS_HDR_MOC} )

# make rules to create c++ source from Qt resource files

# Define include directories for other ibis libs
include_directories( ${IBISLIB_INCLUDE_DIR} )
include_directories( ${IBISLIB_INCLUDE_DIR}/gui )
include_directories( ${IBISLIB_BINARY_DIR} )    # include qt-generated headers

# Generate the file that is going to import the plugins
set( ImportPluginsSourceFile ${CMAKE_CURRENT_BINARY_DIR}/importplugins.cpp )
file( WRITE ${ImportPluginsSourceFile} "#include <QtPlugin>\n\n" )
foreach( plugin ${PluginLibs} )
    file( APPEND ${ImportPluginsSourceFile} "Q_IMPORT_PLUGIN(${plugin}PluginInterface);\n" )
endforeach( plugin )

# Add plugin def for the hardware module
if( IBIS_BUILD_DEFAULT_HARDWARE_MODULE )
    file( APPEND ${ImportPluginsSourceFile} "Q_IMPORT_PLUGIN(IbisHardwareModule);\n" )
endif()
if( IBIS_BUILD_IGSIO_HARDWARE_MODULE )
    file( APPEND ${ImportPluginsSourceFile} "Q_IMPORT_PLUGIN(IbisHardwareIGSIO);\n" )
endif()

#================================
# Configure .desktop files for Linux
#================================
set( DesktopFileIconPath ${CMAKE_CURRENT_SOURCE_DIR}/icons/ibis.png )
set( DesktopFileViewerIconPath ${CMAKE_CURRENT_SOURCE_DIR}/icons/ibis-gray.png )
set( DesktopFilePath ${CMAKE_CURRENT_BINARY_DIR} )
set( DesktopFileExec ${DesktopFilePath}/ibis )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/icons/Ibis.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/Ibis.desktop )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/icons/Ibis-viewer.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/Ibis-viewer.desktop )

#================================
# Define mac osx bundle
#================================
if(APPLE)
    # set how it shows up in the Info.plist file
    set( MACOSX_BUNDLE_INFO_STRING "Ibis" )
    set( MACOSX_BUNDLE_BUNDLE_NAME "Ibis" )
    set( MACOSX_BUNDLE_ICON_FILE "ibis.icns" )
    set( MACOSX_BUNDLE_VERSION ${IBIS_MAJOR_VERSION}.${IBIS_MINOR_VERSION}.${IBIS_PATCH_VERSION} )
    set( MACOSX_BUNDLE_SHORT_VERSION_STRING ${IBIS_MAJOR_VERSION}.${IBIS_MINOR_VERSION}.${IBIS_PATCH_VERSION} )
    set( MACOSX_BUNDLE_LONG_VERSION_STRING "${IBIS_MAJOR_VERSION}.${IBIS_MINOR_VERSION}.${IBIS_PATCH_VERSION} - ${IBIS_VERSION_QUALIFIER} rev. ${IBIS_GIT_HASH}" )
    set( MACOSX_BUNDLE_COPYRIGHT "2002-2011 McConnell Brain Imaging Centre" )
    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/IbisInfo.plist.in ${CMAKE_CURRENT_BINARY_DIR}/IbisInfo.plist )

    # set where in the bundle to put the icns file
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/icons/ibis.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    # include the icns file in the target
    set( IBIS_SRC_ICNS ${CMAKE_CURRENT_SOURCE_DIR}/icons/ibis.icns )

    # make sure we create an app bundle and not a standalone executable
    set( executable_flags MACOSX_BUNDLE )
endif(APPLE)

#================================
# Define executable
#================================
SET( SOURCE ${IBIS_HDR} ${IBIS_HDR_MOC} ${IBIS_SRC} ${ImportPluginsSourceFile} ${IBIS_MOC} ${IBIS_SRC_ICNS} )
ADD_EXECUTABLE( ${exec_name} ${executable_flags} ${SOURCE} )

if (APPLE)
    set_target_properties( ${exec_name} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/IbisInfo.plist )
endif()

set( itkLibsToUse ${ITK_LIBRARIES} )

set( itkLibsToUse ${itkLibsToUse} itkRegistration )


IF( OPENCL_FOUND )
    set( itkLibsToUse ${itkLibsToUse} itkRegistrationOpenCL)
    include_directories( ${ITK_REGISTRATION_OPENCL_INCLUDE_DIR} )
    foreach(GPUKernel ${ITKGPUOrientationMatching_Kernels}) 
        message(${ITK_REGISTRATION_OPENCL_INCLUDE_DIR}/${GPUKernel})
        configure_file(${ITK_REGISTRATION_OPENCL_INCLUDE_DIR}/${GPUKernel}  ${CMAKE_BINARY_DIR}/${GPUKernel} COPYONLY)
    endforeach(GPUKernel)    

    set( itkLibsToUse ${itkLibsToUse} itkVolumeReconstructionOpenCL)
    include_directories( ${ITK_VOLUME_RECONSTRUCTION_OPENCL_INCLUDE_DIR} )
    
    set( gpuLibs ${OPENCL_LIBRARIES})
ENDIF()


set( libsToUse IbisLib  vtkQt  vtkMNI vtkExtensions )

if( USE_POINT_GREY_CAMERA )
    set( flycaptureLib flycapture )
endif()

TARGET_LINK_LIBRARIES( ${exec_name} ${PluginLibs} ${PluginExternalLibs} ${HardwareModulesLibs} ${libsToUse} ${QT_LIBRARIES}
                                    ${VTK_LIBRARIES} ${MINC4ITK_LIBRARIES}
                                    ${itkLibsToUse} ${OpenCV_LIBS} ${flycaptureLib} svl ${gpuLibs} )
qt5_use_modules( ${exec_name} ${IbisQtModules} )

#================================
# Install
#================================

# install executable(s)
install( TARGETS ${exec_name} DESTINATION . )

# fixup the bundle under linux (copy qt libs in the install dir and strip rpaths) 
# On mac, we need to run macdeployqt on the installed bundle (should eventually implement a better solution)
if( NOT APPLE )
	install( PROGRAMS IbisView DESTINATION . )
	install( PROGRAMS IbisNav DESTINATION . )
	
	set( APPS \${CMAKE_INSTALL_PREFIX}/${exec_name} )
	list( APPEND libSearchDirs ${QT_LIBRARY_DIR})
	INSTALL(CODE "include(BundleUtilities)
		fixup_bundle(\"${APPS}\" \"${additionalLib}\" \"${libSearchDirs}\")" COMPONENT Runtime)
endif( NOT APPLE )

#================================
# Packaging
#================================
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ibis image-guided neurosurgery program and viewer")
set( CPACK_PACKAGE_NAME ${exec_name} )
set( CPACK_PACKAGE_CONTACT "Simon Drouin and Anka Kochanowska")
set( CPACK_PACKAGE_VENDOR "Image Processing Lab - McConnell Brain Imaging Centre")
set( CPACK_PACKAGE_VERSION_MAJOR ${IBIS_MAJOR_VERSION})
set( CPACK_PACKAGE_VERSION_MINOR ${IBIS_MINOR_VERSION})
set( CPACK_PACKAGE_VERSION_PATCH ${IBIS_PATCH_VERSION})
if( APPLE )
    set( CPACK_GENERATOR "DragNDrop" )
else( APPLE )
    set( CPACK_GENERATOR "TGZ")
endif( APPLE )
set( CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${IBIS_VERSION_QUALIFIER}-${IBIS_BUILD_QUALIFIER}-${IBIS_GIT_HASH_SHORT}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
