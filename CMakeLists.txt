CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ise-default CXX)

SET(ISE_SRCS
    src/main.cpp
    src/common.cpp
    src/ise.cpp
    src/languages.cpp
    src/config.cpp
    src/option.cpp
    src/sdk/ise_lang_table.cpp
    src/sdk/sdk.cpp
    src/sdk/sdk_option.cpp
)

SET(ISE_PACKAGE ${PROJECT_NAME})
SET(ISE_PKGNAME ${PACKAGE})
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(ISE_LIBDIR "${PREFIX}/lib/scim-1.0/1.4.0/Helper")
SET(ISE_RESDIR "${PREFIX}/share/isf/ise/ise-default/720x1280/default")
SET(MANIFEST_DIR "${PREFIX}/share/packages")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
    SET(CMAKE_BUILD_TYPE "Release")
ENDIF()
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src )

INCLUDE(FindPkgConfig)
pkg_check_modules(ISE_PKGS REQUIRED
        elementary
        efl-assist
        sensor
        dlog
        libscl-ui
        ecore-imf
        isf
        vconf
        libxml-2.0
        )

FOREACH(flag ${ISE_PKGS_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS} -fpie -Wall")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS} -finstrument-functions")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")

ADD_DEFINITIONS("-DPACKAGE=\"${ISE_PACKAGE}\"")
ADD_DEFINITIONS("-DPACKAGE_NAME=\"${ISE_PKGNAME}\"")

#ADD_EXECUTABLE(${PROJECT_NAME} ${ISE_SRCS})
ADD_LIBRARY(${PROJECT_NAME} SHARED ${ISE_SRCS})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${ISE_PKGS_LDFLAGS})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${ISE_LIBDIR})

INSTALL(FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.xml DESTINATION
		${MANIFEST_DIR})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/data/pixmaps/default/sdk/
		DESTINATION ${ISE_RESDIR}/sdk/image)
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/data/layout/sdk/ DESTINATION
        ${ISE_RESDIR}/sdk FILES_MATCHING PATTERN "*.xml")

ADD_SUBDIRECTORY(po)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ise-default-setup CXX)

SET(SETUP_SRCS
    src/setup_module.cpp
    src/option.cpp
    src/languages.cpp
    src/config.cpp
    src/sdk/ise_lang_table.cpp
    src/sdk/sdk_option.cpp
)
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(SETUP_LIBDIR "${PREFIX}/lib/scim-1.0/1.4.0/SetupUI")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
    SET(CMAKE_BUILD_TYPE "Release")
ENDIF()
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

INCLUDE(FindPkgConfig)
pkg_check_modules(SETUP_PKGS REQUIRED
        elementary
        efl-assist
        sensor
        dlog
        libscl-ui
        ecore-imf
        libxml-2.0
        isf
        )

FOREACH(flag ${SETUP_PKGS_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS} -fpie -Wall -DSETUP_MODULE")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS} -finstrument-functions")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SETUP_SRCS})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SETUP_PKGS_LDFLAGS})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${SETUP_LIBDIR})
