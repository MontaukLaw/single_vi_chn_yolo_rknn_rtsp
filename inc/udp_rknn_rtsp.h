#ifndef __UDP_RKNN_RTSP_H_
#define __UDP_RKNN_RTSP_H_

#include "opencv2/opencv.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <dlfcn.h>

#define _BASETSD_H

#include "im2d.h"
#include "rga.h"
#include "drm_func.h"
#include "rga_func.h"
#include "rknn_api.h"
#include "rkmedia_api.h"
#include "postprocess.h"
#include "sample_common.h"

#ifdef __cplusplus
}
#endif

#endif
