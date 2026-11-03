project(skvf-ext)

file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_SOURCE_DIR}/src.zip" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}")

set(GLFW_INSTALL OFF)
set(GLFW_BUILD_DOCS OFF)
message(STATUS "[glfw]")
add_subdirectory(src/glfw)
add_library(glfw::glfw ALIAS glfw)

set(GLM_ENABLE_CXX_20 ON)
message(STATUS "[glm]")
add_subdirectory(src/glm)
target_compile_definitions(glm PUBLIC
  GLM_FORCE_XYZW_ONLY
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_SILENT_WARNINGS
  GLM_ENABLE_EXPERIMENTAL
  GLM_EXT_INCLUDED
)

message(STATUS "[Vulkan-HPP]")
add_subdirectory(src/vulkan-hpp)

message(STATUS "[VulkanMemoryAllocator]")
add_subdirectory(src/vma)
set_target_properties(VulkanMemoryAllocator PROPERTIES
    SYSTEM TRUE
)

message(STATUS "[klib]")
add_subdirectory(src/klib)

message(STATUS "[gsl]")
add_subdirectory(src/gsl)