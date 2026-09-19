# CMakeList.txt : CMake project for Axio, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Axio "Main.cpp" "Application.h" "Application.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Axio PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.


add_library(GLFW STATIC IMPORTED)
set_target_properties(GLFW PROPERTIES IMPORTED_LOCATION  "${LIB_DIR}/glfw-3.4/lib-vc-2022/glfw3.lib" )
target_include_directories(GLFW INTERFACE "${LIB_DIR}/glfw-3.4/include")

add_library(GLAD "${LIB_DIR}/OpenGL/src/glad.c")
target_include_directories(GLAD PUBLIC "${LIB_DIR}/OpenGL/include")

target_include_directories(Axio PRIVATE
    "${LIB_DIR}/ImGui"         
    "${LIB_DIR}/ImGui/backends"
)