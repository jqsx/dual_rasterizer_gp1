# Source files
set(SOURCES 
    "src/main.cpp"
    "src/LeakDetector.cpp"
    "src/Matrix.cpp"
    "src/Renderer.cpp"
    "src/Timer.cpp"
	"src/Vector2.cpp"
    "src/Vector3.cpp"
    "src/Vector4.cpp"
)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# DirectX11
option(DIRECTX_11_ENABLED "Enable DirectX 11 Support" ON)
if(DIRECTX_11_ENABLED)
    find_library(DXGI_LIBRARY dxgi.lib)
    find_library(D3D11_LIBRARY d3d11.lib)
    if(DXGI_LIBRARY AND D3D11_LIBRARY)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${DXGI_LIBRARY} ${D3D11_LIBRARY})
    else()
        message(FATAL_ERROR "DirectX libraries not found")
    endif()
endif()

# Copy resources to output folder
set(RESOURCES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
file(GLOB_RECURSE RESOURCE_FILES
    "${RESOURCES_SOURCE_DIR}/*.jpg"
    "${RESOURCES_SOURCE_DIR}/*.png"
    "${RESOURCES_SOURCE_DIR}/*.obj"
    "${RESOURCES_SOURCE_DIR}/*.fx"
)
set(RESOURCES_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/resources/")
file(MAKE_DIRECTORY ${RESOURCES_OUT_DIR})
foreach(RESOURCE ${RESOURCE_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${RESOURCE}
    ${RESOURCES_OUT_DIR})
endforeach(RESOURCE)

# Simple Directmedia Layer
set(SDL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL2-2.30.7")
add_library(SDL STATIC IMPORTED)
set_target_properties(SDL PROPERTIES
    IMPORTED_LOCATION "${SDL_DIR}/lib/x64/SDL2.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL_DIR}/include"
)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL)

file(GLOB_RECURSE DLL_FILES
    "${SDL_DIR}/lib/x64/*.dll"
    "${SDL_DIR}/lib/x64/*.manifest"
)

foreach(DLL ${DLL_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${DLL}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endforeach(DLL)

# Simple Directmedia Layer Image
set(SDL_IMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL2_image-2.8.2")
add_library(SDL_IMAGE STATIC IMPORTED)
set_target_properties(SDL_IMAGE PROPERTIES
    IMPORTED_LOCATION "${SDL_IMAGE_DIR}/lib/x64/SDL2_image.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL_IMAGE_DIR}/include"
)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL_IMAGE)

file(GLOB_RECURSE DLL_FILES
    "${SDL_IMAGE_DIR}/lib/x64/*.dll"
    "${SDL_IMAGE_DIR}/lib/x64/*.manifest"
)

foreach(DLL ${DLL_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${DLL}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endforeach(DLL)

# DirectX Effects
if(DIRECTX_11_ENABLED)
    set(FX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/dx11effects")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(FX_LIBRARY "${FX_DIR}/lib/x64/dx11effects_d.lib")
    else()
        set(FX_LIBRARY "${FX_DIR}/lib/x64/dx11effects.lib")
    endif()
    add_library(FX STATIC IMPORTED)
    set_target_properties(FX PROPERTIES
        IMPORTED_LOCATION "${FX_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FX_DIR}/include"
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE FX)
endif()