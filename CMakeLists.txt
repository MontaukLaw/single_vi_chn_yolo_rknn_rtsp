cmake_minimum_required(VERSION 3.24)
project(double_vi_test)

set(CMAKE_CXX_STANDARD 11)

set(TOOLCHAIN_DIR /opt/atk-dlrv1126-toolchain)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/usr/bin/arm-linux-gnueabihf-g++)
set(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/usr/bin/arm-linux-gnueabihf-gcc)
set(SYSROOT ${TOOLCHAIN_DIR}/arm-buildroot-linux-gnueabihf/sysroot/usr/include)
set(CMAKE_SYSROOT ${TOOLCHAIN_DIR}/arm-buildroot-linux-gnueabihf/sysroot)
set(INC_DIR ${PROJECT_SOURCE_DIR}/inc)
set(SYS_LIB_INC ${TOOLCHAIN_DIR}/prebuilts/gcc/linux-x86/arm/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/arm-linux-gnueabihf/include/)

set(X11_LIB_INC /usr/include)
message("x11: ${X11_LIB_INC}")

add_definitions(-g -O0 -ggdb -gdwarf -funwind-tables -rdynamic)
add_definitions(-Wno-write-strings -Wno-return-type)

set(OPENCV_LIBS opencv_core opencv_imgcodecs opencv_imgproc opencv_features2d opencv_flann opencv_highgui opencv_freetype)
set(RKNN_LIBS easymedia rga rkaiq rknn_api sample_common_isp)

# add rtsp
set(RTSP_LIBS rtsp)

# for libs
link_directories(include libs)

# for my own include
include_directories(${INC_DIR})

# for 3 party libs head files
include_directories(${SYSROOT})
include_directories(${SYSROOT}/rga)
include_directories(${SYSROOT}/easymedia)
include_directories(${SYSROOT}/rkaiq/uAPI)
include_directories(${SYSROOT}/rkaiq/xcore)
include_directories(${SYSROOT}/rkaiq/algos)
include_directories(${SYSROOT}/rkaiq/common)
include_directories(${SYSROOT}/rkaiq/iq_parser)
include_directories(${SYSROOT}/rknn)

include_directories(.)
include_directories(${INC_DIR}/3rdparty)
include_directories(${INC_DIR}/3rdparty/drm/include/libdrm)
include_directories(${INC_DIR}/3rdparty/rga/include)

include_directories(${X11_LIB_INC})

add_definitions(-DRKAIQ)

add_executable(single_vi_yolo_rknn_rtsp src/main.cpp
        src/rga_trans.c
        src/comm.cc
        src/drm_func.c
        src/rknn_funcs.cpp
        src/rknn_model.cc
        src/sample_common_isp.c
        src/yolo.cc)

target_link_libraries(single_vi_yolo_rknn_rtsp ${OPENCV_LIBS} ${RKNN_LIBS} pthread dl ${RTSP_LIBS})
