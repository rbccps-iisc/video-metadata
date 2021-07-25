cmake_minimum_required(VERSION 3.19)
project(video_metadata)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)

include(FindPackageHandleStandardArgs)

find_package(PkgConfig)
pkg_check_modules(PC_SWSCALE libswscale)
set(SWSCALE_DEFINITIONS ${PC_SWSCALE_CFLAGS_OTHER})

find_library(SWSCALE_LIBRARIES swscale
        HINTS ${PC_SWSCALE_LIBDIR} ${PC_SWSCALE_LIBRARY_DIRS}
        )

find_path(SWSCALE_INCLUDE_DIR swscale.h
        HINTS ${PC_SWSCALE_INCLUDEDIR} ${PC_SWSCALE_INCLUDE_DIRS}
        PATH_SUFFIXES libswscale
        )

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY avutil)

find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
find_library(AVDEVICE_LIBRARY avdevice)

find_package( OpenCV 4 REQUIRED )
if(OpenCV_FOUND)
    message(STATUS "Found OpenCV version ${OpenCV_VERSION}")
    message(STATUS "OpenCV directories: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
    include_directories(${OpenCV_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "OpenCV not found, please read the README.md")
endif(OpenCV_FOUND)

add_executable(video_metadata main.cpp)
target_include_directories(video_metadata PRIVATE ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR})
target_link_libraries(video_metadata PRIVATE ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY} ${SWSCALE_LIBRARIES} ${OpenCV_LIBS})