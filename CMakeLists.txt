cmake_minimum_required(VERSION 3.20)
project(Vulkan2D VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVCでUTF-8ソースファイルをサポート
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Vulkan SDK
find_package(Vulkan REQUIRED)

# GLFW for window management
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# Fetch GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# Fetch stb (for image loading)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# Fetch FreeType (for font rendering)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-2
)
FetchContent_MakeAvailable(freetype)
unset(CMAKE_POLICY_VERSION_MINIMUM CACHE)

# Fetch miniaudio (for audio playback)
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
)
FetchContent_MakeAvailable(miniaudio)

# Library source files
set(VULKAN2D_SOURCES
    src/Core/Engine.cpp
    src/Core/Window.cpp
    src/Renderer/VulkanContext.cpp
    src/Renderer/Renderer2D.cpp
    src/Renderer/Pipeline.cpp
    src/Renderer/Shader.cpp
    src/Renderer/Buffer.cpp
    src/Renderer/Texture.cpp
    src/Renderer/SpriteBatch.cpp
    src/Graphics/Sprite.cpp
    src/Graphics/Camera2D.cpp
    src/Graphics/Font.cpp
    src/Graphics/TextRenderer.cpp
    src/Math/Transform2D.cpp
    src/Input/InputManager.cpp
    src/Audio/Sound.cpp
    src/Audio/AudioManager.cpp
)

set(VULKAN2D_HEADERS
    include/Vulkan2D/Vulkan2D.h
    include/Vulkan2D/Core/Engine.h
    include/Vulkan2D/Core/Window.h
    include/Vulkan2D/Renderer/VulkanContext.h
    include/Vulkan2D/Renderer/Renderer2D.h
    include/Vulkan2D/Renderer/Pipeline.h
    include/Vulkan2D/Renderer/Shader.h
    include/Vulkan2D/Renderer/Buffer.h
    include/Vulkan2D/Renderer/Texture.h
    include/Vulkan2D/Renderer/SpriteBatch.h
    include/Vulkan2D/Graphics/Sprite.h
    include/Vulkan2D/Graphics/Camera2D.h
    include/Vulkan2D/Graphics/Font.h
    include/Vulkan2D/Graphics/TextRenderer.h
    include/Vulkan2D/Math/Transform2D.h
    include/Vulkan2D/Input/InputManager.h
    include/Vulkan2D/Audio/Sound.h
    include/Vulkan2D/Audio/SoundData.h
    include/Vulkan2D/Audio/AudioManager.h
)

# Create library
add_library(Vulkan2D STATIC ${VULKAN2D_SOURCES} ${VULKAN2D_HEADERS})

target_include_directories(Vulkan2D 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${stb_SOURCE_DIR}
        ${miniaudio_SOURCE_DIR}
)

target_link_libraries(Vulkan2D 
    PUBLIC 
        Vulkan::Vulkan
        $<BUILD_INTERFACE:glfw>
        $<BUILD_INTERFACE:glm::glm>
    PRIVATE
        $<BUILD_INTERFACE:freetype>
)

target_compile_definitions(Vulkan2D
    PUBLIC
        VK_API_VERSION_1_3
        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# Shader compilation
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin)

if(GLSLC)
    file(GLOB_RECURSE SHADER_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
    )
    
    set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
    
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SHADER_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${GLSLC} ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling shader: ${SHADER_NAME}"
        )
        list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
    endforeach()
    
    add_custom_target(Shaders ALL DEPENDS ${SHADER_OUTPUTS})
    add_dependencies(Vulkan2D Shaders)
endif()

# Install settings
include(GNUInstallDirs)

install(TARGETS Vulkan2D
    EXPORT Vulkan2DTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export without dependencies (they will be handled in Config file)
set_target_properties(Vulkan2D PROPERTIES
    EXPORT_NAME Vulkan2D
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# CMAKE_INSTALL_DEFAULT_COMPONENT_NAME is used to avoid exporting dependencies
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Vulkan2D_Runtime)

install(EXPORT Vulkan2DTargets
    FILE Vulkan2DTargets.cmake
    NAMESPACE Vulkan2D::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Vulkan2D
    EXPORT_LINK_INTERFACE_LIBRARIES
)

unset(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Vulkan2DConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/Vulkan2DConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Vulkan2D
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/Vulkan2DConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/Vulkan2DConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/Vulkan2DConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Vulkan2D
)

# Install shaders
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Vulkan2D/shaders
)

# Example application
option(BUILD_EXAMPLES "Build example applications" ON)
if(BUILD_EXAMPLES)
    add_executable(Example examples/main.cpp)
    target_link_libraries(Example PRIVATE Vulkan2D)
    
    # Copy shaders to example output directory
    add_custom_command(TARGET Example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADER_OUTPUT_DIR}
        $<TARGET_FILE_DIR:Example>/shaders
    )
endif()
