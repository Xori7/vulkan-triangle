cmake_minimum_required(VERSION 3.26)
project(vulkantest C)

set(CMAKE_C_STANDARD 90)

include_directories(C:/VulkanSDK/1.3.268.0/Include)
link_libraries(C:/VulkanSDK/1.3.268.0/Lib/vulkan-1.lib)

add_executable(vulkantest main.c
        shader.vert
        shader.frag)

