CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ise-default)

SET(BASIC_ISE_SRCS
    src/main.cpp
    src/common.cpp
    src/ise.cpp
    src/languages.cpp
    src/config.cpp
    src/option.cpp
    src/imdata.cpp
    src/sdk/ise_lang_table.cpp
    src/sdk/sdk.cpp
    src/sdk/sdk_option.cpp
    src/candidate/candidate-factory.cpp
    src/candidate/candidate.cpp
    src/candidate/efl/candidate-efl.cpp
    src/candidate/efl/candidate-multiline-efl.cpp
)

IF (with_wayland)
    SET(ISE_SRCS ${BASIC_ISE_SRCS} src/input-method-protocol.c)
ELSE (with_wayland)
    SET(ISE_SRCS ${BASIC_ISE_SRCS})
ENDIF (with_wayland)

SET(ISE_PACKAGE ${PROJECT_NAME})
SET(ISE_PKGNAME ${PACKAGE})
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(LIBDIR ${LIB_INSTALL_DIR})
SET(ISE_LIBDIR "${LIBDIR}/scim-1.0/1.4.0/Helper")
SET(ISE_RESDIR "${PREFIX}/share/isf/ise/ise-default/720x1280/default")
SET(MANIFEST_DIR "${PREFIX}/share/packages")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
    SET(CMAKE_BUILD_TYPE "Release")
ENDIF()
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/candidate/include
    ${CMAKE_SOURCE_DIR}/src/candidate/efl
)

INCLUDE(FindPkgConfig)
SET(PKGS_CHECK_MODULES
        elementary
        efl-extension
        dlog
        libscl-ui
        ecore-imf
        isf
        vconf
        libxml-2.0
        )

IF (with_wayland)
    pkg_check_modules(ISE_PKGS REQUIRED ${PKGS_CHECK_MODULES} ecore-wayland wayland-client)
ELSE (with_wayland)
    pkg_check_modules(ISE_PKGS REQUIRED ${PKGS_CHECK_MODULES})
ENDIF (with_wayland)

FOREACH(flag ${ISE_PKGS_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS} -fPIC -Wall")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS} -finstrument-functions")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")

ADD_DEFINITIONS("-DPACKAGE=\"${ISE_PACKAGE}\"")
ADD_DEFINITIONS("-DPACKAGE_NAME=\"${ISE_PKGNAME}\"")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${ISE_SRCS})

# For edc File
ADD_CUSTOM_TARGET(
    candidate-single.edj
    COMMAND edje_cc
        -id ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/candidate-single
        -sd ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/candidate-single
        ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/candidate-single/candidate.edc
        ${CMAKE_BINARY_DIR}/candidate-single.edj
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/candidate-single/candidate.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} candidate-single.edj)
INSTALL(FILES ${CMAKE_BINARY_DIR}/candidate-single.edj DESTINATION ${ISE_RESDIR}/sdk/edc)
# For edc File end
# For edc File
ADD_CUSTOM_TARGET(
    candidate-multiline.edj
    COMMAND edje_cc
        -id ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/candidate-multiline
        -sd ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/candidate-multiline
        ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/candidate-multiline/candidate.edc
        ${CMAKE_BINARY_DIR}/candidate-multiline.edj
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/candidate-multiline/candidate.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} candidate-multiline.edj)
INSTALL(FILES ${CMAKE_BINARY_DIR}/candidate-multiline.edj DESTINATION ${ISE_RESDIR}/sdk/edc)
# For edc File end

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

IF (with_wayland)
        ADD_DEFINITIONS("-DWAYLAND")
ENDIF(with_wayland)

SET(SETUP_SRCS
    src/setup_module.cpp
    src/option.cpp
    src/languages.cpp
    src/config.cpp
    src/sdk/ise_lang_table.cpp
    src/sdk/sdk_option.cpp
)
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(SETUP_LIBDIR "${LIBDIR}/scim-1.0/1.4.0/SetupUI")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
    SET(CMAKE_BUILD_TYPE "Release")
ENDIF()
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

INCLUDE(FindPkgConfig)
pkg_check_modules(SETUP_PKGS REQUIRED
        elementary
        efl-extension
        dlog
        libscl-ui
        ecore-imf
        libxml-2.0
        isf
        )

FOREACH(flag ${SETUP_PKGS_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS} -fPIC -Wall -DSETUP_MODULE")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS} -finstrument-functions")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SETUP_SRCS})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SETUP_PKGS_LDFLAGS})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${SETUP_LIBDIR})
