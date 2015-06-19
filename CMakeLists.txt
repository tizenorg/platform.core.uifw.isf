CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ise-default CXX)

SET(ISE_SRCS
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

    src/ise-emoticon-list.cpp
    src/ise-emoticon-mode.cpp
)

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
pkg_check_modules(ISE_PKGS REQUIRED
        elementary
        efl-extension
        dlog
        libscl-ui
        libscl-core
        ecore-imf
        isf
        vconf
        libxml-2.0
        )

FOREACH(flag ${ISE_PKGS_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS} -fPIC -Wall")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS} -finstrument-functions")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")

ADD_DEFINITIONS("-DPACKAGE=\"${ISE_PACKAGE}\"")
ADD_DEFINITIONS("-DPACKAGE_NAME=\"${ISE_PKGNAME}\"")
ADD_DEFINITIONS(-DISE_NAME=\"/ise-default\")
ADD_DEFINITIONS(-DSUPPORTS_EMOTICONS)

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
# For edc File
ADD_CUSTOM_TARGET(
    customised_gengrid.edj
    COMMAND edje_cc -no-save
        -id ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/gengrid
        -sd ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/gengrid
        ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/gengrid/customised_gengrid.edc
        ${CMAKE_BINARY_DIR}/customised_gengrid.edj
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/gengrid/customised_gengrid.edc)
ADD_DEPENDENCIES(${PROJECT_NAME} customised_gengrid.edj)
INSTALL(FILES ${CMAKE_BINARY_DIR}/customised_gengrid.edj DESTINATION ${ISE_RESDIR}/sdk/edc)

ADD_CUSTOM_TARGET(
    layout_keypad.edj
    COMMAND edje_cc -no-save
        ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/gengrid/layout_keypad.edc
        ${CMAKE_BINARY_DIR}/layout_keypad.edj
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/data/edc/gengrid/layout_keypad.edc)
ADD_DEPENDENCIES(${PROJECT_NAME} layout_keypad.edj)
INSTALL(FILES ${CMAKE_BINARY_DIR}/layout_keypad.edj DESTINATION ${ISE_RESDIR}/sdk/edc)
# For edc File end

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${ISE_PKGS_LDFLAGS})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${ISE_LIBDIR})

INSTALL(FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.xml DESTINATION
        ${MANIFEST_DIR})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/data/pixmaps/default/sdk/
        DESTINATION ${ISE_RESDIR}/sdk/image)
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/data/pixmaps/default/tv/
        DESTINATION ${ISE_RESDIR}/tv/image)
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/data/layout/sdk/ DESTINATION
        ${ISE_RESDIR}/sdk FILES_MATCHING PATTERN "*.xml")
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/data/layout/tv/ DESTINATION
        ${ISE_RESDIR}/tv FILES_MATCHING PATTERN "*.xml")

ADD_SUBDIRECTORY(po)
