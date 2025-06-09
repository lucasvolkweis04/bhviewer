cmake_minimum_required(VERSION 2.8)

cmake_policy(SET CMP0072 NEW)
project(bvhviewer)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
add_executable(${PROJECT_NAME} main.c opengl.c)
target_link_libraries(bvhviewer PRIVATE GLUT::GLUT OpenGL::GL OpenGL::GLU m)
