project(${package_name}-lib VERSION ${build_version})

add_library(${PROJECT_NAME})
add_library(${package_name}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC
    Vulkan::Hpp
    VulkanMemoryAllocator
    glm::glm
    glfw::glfw
    klib::klib
    GSL
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
  GLFW_INCLUDE_VULKAN
  VK_NO_PROTOTYPES
)

set(version_header "${CMAKE_CURRENT_BINARY_DIR}/include/${package_name}/build_version.hpp")
configure_file(src/build_version.hpp.in "${version_header}" @ONLY)

file(GLOB_RECURSE headers LIST_DIRECTORIES false "include/*.hpp")
file(GLOB_RECURSE sources LIST_DIRECTORIES false "src/*.cpp")

target_sources(${PROJECT_NAME} 
  PUBLIC FILE_SET HEADERS
    BASE_DIRS include "${CMAKE_CURRENT_BINARY_DIR}/include"
    FILES ${headers} "${version_header}"
  PRIVATE
    ${sources}
)