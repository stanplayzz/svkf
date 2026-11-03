cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(version.cmake)
set(package_name svkf)

project(${package_name} VERSION ${build_version} LANGUAGES CXX)

add_subdirectory(ext)
add_subdirectory(lib)
add_subdirectory(example)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type
        $<$<NOT:$<CONFIG:Debug>>:-Werror>
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        $<$<NOT:$<CONFIG:Debug>>:/WX>
    )
endif()