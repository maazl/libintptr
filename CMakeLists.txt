CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# set default build type to Release
IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

PROJECT(intptr CXX)
SET_PROPERTY(GLOBAL PROPERTY CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -faligned-new")
FIND_PACKAGE(Threads REQUIRED)

# Place results in make root rather than source dir
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

INCLUDE_DIRECTORIES(include)

ADD_LIBRARY(intptr STATIC src/intptr.cpp)

#ADD_SUBDIRECTORY(src)

# install TARGETS links the executables a second time, taking twice a much time just for nothing.
# install(TARGETS analyze RUNTIME DESTINATION bin)
# work around: use install PROGRAMS
#INSTALL(PROGRAMS $<TARGET_FILE:analyze> $<TARGET_FILE:ref> $<TARGET_FILE:sampledist> DESTINATION bin)

ENABLE_TESTING()
ADD_SUBDIRECTORY(test EXCLUDE_FROM_ALL)
