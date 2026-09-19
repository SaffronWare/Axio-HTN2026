# CMakeList.txt : CMake project for Axio, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Axio "Main.cpp" "Application.h" "Application.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Axio PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
