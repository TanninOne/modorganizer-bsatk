CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

SET(PROJ_AUTHOR Tannin)
SET(PROJ_ARCH x86)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  SET(PROJ_ARCH x64)
endif()


SET(bsatk_SRCS
    filehash.cpp
    bsafile.cpp
    bsaexception.cpp
    bsafolder.cpp
    bsaarchive.cpp
    bsatypes.cpp
  )

SET(bsatk_HDRS
    filehash.h
    errorcodes.h
    bsatypes.h
    bsafile.h
    bsafolder.h
    bsaexception.h
    bsaarchive.h
  )

FIND_PACKAGE(zlib REQUIRED)

INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS}
                    ${ZLIB_INCLUDE_DIRS}/build) # in case of an out-of-source build

ADD_LIBRARY(bsatk STATIC ${bsatk_HDRS} ${bsatk_SRCS})

IF (NOT "${OPTIMIZE_COMPILE_FLAGS}" STREQUAL "")
  SET_TARGET_PROPERTIES(bsatk PROPERTIES COMPILE_FLAGS_RELWITHDEBINFO
                        ${OPTIMIZE_COMPILE_FLAGS})
ENDIF()
SET_TARGET_PROPERTIES(bsatk PROPERTIES LINK_FLAGS_RELWITHDEBINFO
                      "/LARGEADDRESSAWARE ${OPTIMIZE_LINK_FLAGS}")

###############
## Installation

INSTALL(TARGETS bsatk
        ARCHIVE DESTINATION libs)
