FIND_PATH(ISL_INCLUDE_DIR isl_libs.h PATH_SUFFIXES isl)

FIND_LIBRARY(ISL_LIBRARY NAMES isl)

IF (ISL_INCLUDE_DIR AND ISL_LIBRARY)
  SET(ISL_FOUND TRUE)
ENDIF (ISL_INCLUDE_DIR AND ISL_LIBRARY)


IF (ISL_FOUND)
  IF (NOT Isl_FIND_QUIETLY)
    MESSAGE(STATUS "Found Isl: ${ISL_LIBRARY}")
  ENDIF (NOT Isl_FIND_QUIETLY)
ELSE (ISL_FOUND)
  IF (Isl_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find Isl")
  ENDIF (Isl_FIND_REQUIRED)
ENDIF (ISL_FOUND)

