PROJECT( svl )

IF(UNIX)
	SET( VL_HAS_ABSF 1 )
	SET( VL_HAS_DRAND 1 )
	SET( VL_USE_MEMCPY 1 )
ENDIF(UNIX)
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/SVLConfigBase.h ${CMAKE_CURRENT_BINARY_DIR}/SVLConfig.h )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} )

SET( SVL_SRC  Basics.cpp LibSVL.cpp Box2i.cpp Box2d.cpp )
SET( SVL_HDR Basics.h Constants.h Mat2.h Mat3.h Mat4.h Mat.h SVLgl.h Transform.h Utils.h Vec2.h Vec3.h Vec4.h Vec.h Box2i.h Box2d.h )

ADD_LIBRARY( svl ${SVL_SRC} ${SVL_HDR} )

#================================
# Define include dir for
# dependent projects
#================================
SET( include_dir_content ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} )  # we need binary dir for configured header files
SET( svlIncludeDir ${include_dir_content} CACHE PATH "Include directory for svl .h files" )
MARK_AS_ADVANCED( svlIncludeDir )

INSTALL_FILES( /include/svl .*\\.h$ )
INSTALL_TARGETS( /lib/ svl )
