cmake_minimum_required(VERSION 2.8)
project(ONIEXTRACTOREXAMPLE)

# compile with C++11 support
add_definitions("-std=c++0x -Dlinux -D__STANDALONE_ONI_EXTRACTOR")

# OpenCV
set(LIBS ${LIBS} opencv_core opencv_imgproc)
set(LIBS ${LIBS} opencv_highgui)

# OpenNI
set(OPENNI_INCLUDE_DIRS "$ENV{OPEN_NI_INSTALL_PATH}/usr/include/ni")
include_directories(${OPENNI_INCLUDE_DIRS})

find_library(OPEN_NI_LIBRARY NAMES OpenNI PATHS $ENV{OPEN_NI_INSTALL_PATH}/usr/lib)
SET(LIBS ${LIBS} ${OPEN_NI_LIBRARY})

add_executable(oniExtractorExample OniExtractor.cpp)
target_link_libraries(oniExtractorExample ${LIBS})
