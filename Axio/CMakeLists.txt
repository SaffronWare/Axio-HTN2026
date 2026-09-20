file(GLOB SOURCES "Main.cpp" "Application.h" "Application.cpp" "${LIB_DIR}/ImGui/backends/*.h" "${LIB_DIR}/ImGui/backends/*.cpp" 
"${LIB_DIR}/ImGui/*.cpp" "${LIB_DIR}/ImGui/*.h" "${LIB_DIR}/imgui-node-editor-master/*.h" "${LIB_DIR}/imgui-node-editor-master/*.cpp")


add_executable (Axio ${SOURCES} "NodeEditor.h" )

add_library(GLFW STATIC IMPORTED)
set_target_properties(GLFW PROPERTIES IMPORTED_LOCATION  "${LIB_DIR}/glfw-3.4/lib-vc-2022/glfw3.lib" )
target_include_directories(GLFW INTERFACE "${LIB_DIR}/glfw-3.4/include")

add_library(GLAD "${LIB_DIR}/OpenGL/src/glad.c" "NodeEditor.h" )
target_include_directories(GLAD PUBLIC "${LIB_DIR}/OpenGL/include")

target_include_directories(Axio PRIVATE
    "${LIB_DIR}/ImGui"         
    "${LIB_DIR}/ImGui/backends"
    "${LIB_DIR}/imgui-node-editor-master"
)

target_link_libraries(Axio PUBLIC GLAD)
target_link_libraries(Axio PRIVATE GLFW)


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Axio PROPERTY CXX_STANDARD 20)
endif()

