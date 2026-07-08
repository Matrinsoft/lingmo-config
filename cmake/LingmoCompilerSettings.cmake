# Compiler settings for Lingmo projects
if(MSVC)
    add_compile_options(/W4 /WX /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    endif()
endif()

# Visibility
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
