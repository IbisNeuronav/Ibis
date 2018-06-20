# at least for now, we will provide all plugins with the include dirs they might need
include_directories( ${IBISLIB_INCLUDE_DIR} )
include_directories( ${IBISLIB_BINARY_DIR} )    # include qt-generated headers

# Needed to build static plugins properly
add_definitions( -DQT_STATICPLUGIN )

# Macro used by plugin declaration files
macro( DeclarePlugin pluginName OnByDefault )
    set( ibisPluginName ${pluginName} )
    set( ${ibisPluginName}Option IBIS_PLUGINS_BUILD_${ibisPluginName} )
    option( ${${ibisPluginName}Option} "Build ${pluginName} plugin. $" ${OnByDefault} )
    set( ${ibisPluginName}Declared TRUE )
    set( _doing "" )
    foreach( arg ${ARGN} )
        if( "${arg}" MATCHES "^(DEPENDS|DESCRIPTION)$")
            set(_doing "${arg}")
        elseif("${_doing}" MATCHES "^DEPENDS$")
            list(APPEND ${ibisPluginName}Dependencies "${arg}")
        elseif("${_doing}" MATCHES "^DESCRIPTION$" )
            list( APPEND ${ibisPluginName}Description "${arg}")
        endif()
    endforeach()
    set( ${ibisPluginName}Option IBIS_PLUGINS_BUILD_${ibisPluginName} )
    option( ${${ibisPluginName}Option} "Build ${pluginName} plugin. ${${ibisPluginName}Description}" ${OnByDefault} )
    set( ${ibisPluginName}Declared TRUE )
endmacro()

# 1 - Find all plugin decaration cmake files
set( allIbisPlugins )
file( GLOB allPluginDeclareFiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*/declare-plugin.cmake" )
# find all extra plugin directories
get_filename_component( baseIbisDir ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY )
file( GLOB extraPluginDeclareFiles "${baseIbisDir}/IbisPluginsExtra*/*/declare-plugin.cmake" )
list( APPEND allPluginDeclareFiles ${extraPluginDeclareFiles} )
foreach( pluginDeclareFile ${allPluginDeclareFiles} )
    include( ${pluginDeclareFile} )
    list( APPEND allIbisPlugins ${ibisPluginName} )
    get_filename_component( ${ibisPluginName}BaseDir ${pluginDeclareFile} PATH)
endforeach()

# 2 - Validate the plugin dependencies and make sure there are no cycles.
macro( CheckPlugin pluginName neededBy stack )
  if( NOT DEFINED ${pluginName}Declared )
    message( FATAL_ERROR "Plugin ${neededBy} depends on plugin ${pluginName} which is not available." )
  elseif( NOT ${${pluginName}Option} )
    message( FATAL_ERROR "Plugin ${neededBy} depends on plugin ${pluginName} which is not enabled." )
  endif()
  if(check_started_${pluginName} AND NOT check_finished_${pluginName})
    # We reached a module while traversing its own dependencies recursively.
    set(msg "")
    foreach(entry ${stack})
      set(msg " ${entry} =>${msg}")
      if("${entry}" STREQUAL "${pluginName}")
        break()
      endif()
    endforeach()
    message( FATAL_ERROR "Plugin dependency cycle detected:\n ${msg} ${pluginName}" )
  elseif( NOT check_started_${pluginName} )
    # Traverse dependencies of this module.  Mark the start and finish.
    set(check_started_${pluginName} 1)
    foreach(dep IN LISTS ${pluginName}Dependencies )
      CheckPlugin( ${dep} ${pluginName} "${pluginName};${stack}")
    endforeach()
    set(check_finished_${pluginName} 1)
  endif()
endmacro()

set( allEnabledIbisPlugins )
foreach( ibisPlugin ${allIbisPlugins} )
    if( ${${${ibisPlugin}Option}} )
        CheckPlugin("${ibisPlugin}" "" "")
	list( APPEND allIbisEnabledPlugins ${ibisPlugin} )
    endif()
endforeach()

# 3 - Order list to satisfy dependencies.
include( ${CMAKE_CURRENT_SOURCE_DIR}/TopologicalSort.cmake )
topological_sort( allIbisEnabledPlugins "" Dependencies )

# 4 - Now really run each plugin's CMakeLists.txt

# Macro to define each plugin. To be called inside each plugin's CMakeLists.txt
macro( DefinePlugin PluginSrc PluginHdr PluginHdrMoc PluginUi )
    qt5_wrap_ui( PluginHdrUi ${PluginUi} )
    qt5_wrap_cpp( PluginMoc ${PluginHdrMoc} )
    include_directories( ${CMAKE_CURRENT_BINARY_DIR} )  # needed to include qt-generated source files
    set( PluginSources ${PluginSrc} ${PluginHdrMoc} ${PluginUi} ${PluginMoc} ${PluginHdrUi} )
    add_library( ${PluginName} ${PluginSources} )
    target_link_libraries( ${PluginName} ibisVTKHeaders )
    add_library( ${PluginName}Headers INTERFACE )
    target_compile_definitions( ${PluginName}Headers INTERFACE LIBRARY_HEADER_ONLY )
    target_include_directories( ${PluginName}Headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
    if( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
        set_target_properties( ${PluginName} PROPERTIES COMPILE_FLAGS "-fPIC")
    endif()
endmacro( DefinePlugin )

foreach( ibisPlugin ${allIbisEnabledPlugins} )
    set( PluginName ${ibisPlugin} )
    add_subdirectory( ${${ibisPlugin}BaseDir} ${CMAKE_CURRENT_BINARY_DIR}/${PluginName} )
    list( APPEND PluginLibs ${ibisPlugin} ) # assume plugin lib name is the same as dir name
    list( APPEND PluginExternalLibsAll ${PluginExternalLibs} )
endforeach()

# Propagate these variables to parent dir so that Ibis can link with required libs.
set( PluginLibs ${PluginLibs} PARENT_SCOPE )
set( PluginExternalLibs ${PluginExternalLibsAll} PARENT_SCOPE )

