set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Test)

function(add_lingmo_config_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name}
        PRIVATE
            Lingmo::Config
            Qt6::Test
    )
    target_compile_definitions(${name}
        PRIVATE
            DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    )
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_lingmo_config_test(tst_Config)
