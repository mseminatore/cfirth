cmake_minimum_required(VERSION 3.10)

# set the project name
project(firth)

# add the includes
include_directories(${PROJECT_SOURCE_DIR})

# add the executable
add_executable(firth main.c firth.c core.c firth_float.c)