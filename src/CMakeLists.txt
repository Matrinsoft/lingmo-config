set(PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/LingmoConfig/Config.h
    ${CMAKE_SOURCE_DIR}/include/LingmoConfig/ConfigSchema.h
    ${CMAKE_SOURCE_DIR}/include/LingmoConfig/ConfigWatcher.h
)

set(PRIVATE_HEADERS
    private/Config_p.h
    private/ConfigSchema_p.h
    private/ConfigWatcher_p.h
)

set(SOURCES
    Config.cpp
    ConfigSchema.cpp
    ConfigWatcher.cpp
)

set(CMAKE_AUTOMOC ON)

add_library(LingmoConfig SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
add_library(Lingmo::Config ALIAS LingmoConfig)

target_include_directories(LingmoConfig
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(LingmoConfig
    PUBLIC
        Qt6::Core
)

target_compile_definitions(LingmoConfig
    PRIVATE
        LINGMOCONFIG_LIBRARY
)

set_target_properties(LingmoConfig PROPERTIES
    SOVERSION 1
    VERSION ${PROJECT_VERSION}
    DEFINE_SYMBOL LINGMOCONFIG_LIBRARY
    OUTPUT_NAME LingmoConfig
)

configure_file(
    "${CMAKE_SOURCE_DIR}/include/LingmoConfig/LingmoConfigExport.h.in"
    "${CMAKE_BINARY_DIR}/include/LingmoConfig/LingmoConfigExport.h"
)

configure_file(
    "${CMAKE_SOURCE_DIR}/include/LingmoConfig/LingmoConfigVersion.h.in"
    "${CMAKE_BINARY_DIR}/include/LingmoConfig/LingmoConfigVersion.h"
)

install(TARGETS LingmoConfig
    EXPORT LingmoConfigTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/LingmoConfig/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/LingmoConfig
    FILES_MATCHING PATTERN "*.h"
    PATTERN "*.h.in" EXCLUDE
)

install(FILES
    "${CMAKE_BINARY_DIR}/include/LingmoConfig/LingmoConfigExport.h"
    "${CMAKE_BINARY_DIR}/include/LingmoConfig/LingmoConfigVersion.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/LingmoConfig
)

install(EXPORT LingmoConfigTargets
    FILE LingmoConfigTargets.cmake
    NAMESPACE Lingmo::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LingmoConfig
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/LingmoConfigConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/LingmoConfigConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LingmoConfig
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/LingmoConfigConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_BINARY_DIR}/LingmoConfigConfig.cmake"
    "${CMAKE_BINARY_DIR}/LingmoConfigConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LingmoConfig
)
