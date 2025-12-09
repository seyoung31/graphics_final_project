cmake_minimum_required(VERSION 3.16)

# Sets project name
project(final_project LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Sets C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specifies required Qt components
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt6 REQUIRED COMPONENTS OpenGL)
find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)
find_package(Qt6 REQUIRED COMPONENTS Xml)

# Allows you to include files from within those directories, without prefixing their filepaths
include_directories(src)

# Specifies .cpp and .h files to be passed to the compiler
add_executable(${PROJECT_NAME}
    src/main.cpp

    src/realtime.cpp
    src/mainwindow.cpp
    src/settings.cpp
    src/utils/scenefilereader.cpp
    src/utils/sceneparser.cpp
    src/shapes/Shape.cpp
    src/shapes/Cone.cpp
    src/shapes/Cube.cpp
    src/shapes/Sphere.cpp
    src/shapes/Cylinder.cpp
    src/shapes/ObjLoader.cpp
    src/utils/shaderutils.cpp
    src/particles/particles.cpp
    src/camera_path/camerapath.cpp


    src/mainwindow.h
    src/realtime.h
    src/settings.h
    src/utils/scenedata.h
    src/utils/scenefilereader.h
    src/utils/sceneparser.h
    src/utils/shaderloader.h
    src/utils/aspectratiowidget/aspectratiowidget.hpp
    src/shapes/Shape.h
    src/shapes/Cone.h
    src/shapes/Cube.h
    src/shapes/Sphere.h 
    src/shapes/Cylinder.h
    src/shapes/ObjLoader.h
    src/shaderloader.h
    src/utils/shaderutils.h
    src/postprocess.h src/postprocess.cpp
    src/utils/watereffect.h src/utils/watereffect.cpp

)

# GLM: this creates its library and allows you to `#include "glm/..."`
add_subdirectory(glm)

# GLEW: this creates its library and allows you to `#include "GL/glew.h"`
add_library(StaticGLEW STATIC glew/src/glew.c
    .gitignore
)
include_directories(${PROJECT_NAME} PRIVATE glew/include)

# Specifies libraries to be linked (Qt components, glew, etc)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::OpenGLWidgets
    Qt::Xml
    StaticGLEW
)

# Specifies other files
qt6_add_resources(${PROJECT_NAME} "Resources"
    PREFIX
        "/"
    FILES
        resources/shaders/default.frag
        resources/shaders/default.vert
	resources/shaders/texture.frag
    	resources/shaders/texture.vert
        resources/shaders/shadow.frag
        resources/shaders/shadow.vert
        resources/shaders/bump.frag
        resources/shaders/bump.vert
        resources/shaders/particles/particles.frag
        resources/shaders/particles/particles.vert
	resources/images/teal_orange_lut_4x4.png
	resources/images/noir_lut_4x4.png
	resources/images/vaporwave_lut_4x4.png
        resources/images/snowflake_only.png
        resources/images/water_color.png
        resources/images/water_displacement.png
        resources/images/scroll_disp_simple.png
)

# GLEW: this provides support for Windows (including 64-bit)
if (WIN32)
  add_compile_definitions(GLEW_STATIC)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    opengl32
    glu32
  )
endif()

# Set this flag to silence warnings on Windows
if (MSVC OR MSYS OR MINGW)
  set(CMAKE_CXX_FLAGS "-Wno-volatile")
endif()
# Set this flag to silence warnings on MacOS
if (APPLE)
  set(CMAKE_CXX_FLAGS "-Wno-deprecated-volatile")
endif()
