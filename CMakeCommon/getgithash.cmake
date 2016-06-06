# the FindSubversion.cmake module is part of the standard distribution
include(FindGit)


if( EXISTS ${SOURCE_DIR}/.git )
  # extract working copy information for source into IBIS variables
  # Get the latest abbreviated commit hash of the working branch
  execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else( EXISTS ${SOURCE_DIR}/.git )
  set( GIT_COMMIT_HASH 0 )
  set( GIT_COMMIT_HASH_SHORT 0 )
endif( EXISTS ${SOURCE_DIR}/.git )

# creates githash.h using cmake script
  # write a file with the IBIS_GIT_HASH define
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/githash.h.txt "#define IBIS_GIT_HASH \"${GIT_COMMIT_HASH}\"\n#define IBIS_GIT_HASH_SHORT \"${GIT_COMMIT_HASH_SHORT}\"\n")

# copy the file to the final header only if the version changes
# reduces needless rebuilds
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        githash.h.txt githash.h)

