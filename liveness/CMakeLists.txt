CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(liveness)
set(CMAKE_C_FLAGS "-g -Wall -Wunknown-pragmas -std=c99")

#need to include FindOPENCL.cmake to
SET( CMAKE_MODULE_PATH ${liveness_SOURCE_DIR}/../cmake)

message("CMake module path: ${CMAKE_MODULE_PATH}\n")

FIND_PACKAGE (OPENCL)

INCLUDE_DIRECTORIES( 
    ${liveness_SOURCE_DIR}/../opencl11/
)

message("include directories: \n")
get_directory_property(includes INCLUDE_DIRECTORIES)
message("${includes}\n")

#provide the source path so we can open our .cl file at runtime
#ADD_DEFINITIONS(-DCL_SOURCE_DIR="${liveness_SOURCE_DIR}")
#build in debug mode
ADD_DEFINITIONS(-g )



#the source files we want to compile into the library
set (LIBCFILES opencl.c)
#set a CMake variable to name the library
SET(cllib liveness)
ADD_LIBRARY(${cllib} ${LIBCFILES})


#create the executable
SET(EXEC liveness1.x)
ADD_EXECUTABLE(${EXEC} main.cpp)

TARGET_LINK_LIBRARIES (${EXEC}
   ${cllib}
   ${OPENCL_LIBRARIES}
)
