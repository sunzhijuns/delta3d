
SET(LIB_NAME     dtLMS)


SET(HEADER_PATH ${DELTA3D_SOURCE_DIR}/inc/${LIB_NAME})
SET(SOURCE_PATH ${DELTA3D_SOURCE_DIR}/src/${LIB_NAME})
file(GLOB LIB_PUBLIC_HEADERS "${HEADER_PATH}/*.h")
file(GLOB LIB_SOURCES "*.cpp")

SET(LIB_DEPS
 ${DTGAME_LIBRARY}
 )


DELTA3D_ADD_LIBRARY(${LIB_NAME} DT_LMS_LIBRARY)

IF (WIN32)
TARGET_LINK_LIBRARIES(${LIB_NAME} wsock32)
ENDIF (WIN32)
