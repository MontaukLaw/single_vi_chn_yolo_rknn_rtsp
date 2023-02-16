#ifndef __VDEC_H_
#define __VDEC_H_

#include "udp_rknn_rtsp.h"
#include "comm.h"

#ifdef __cplusplus
extern "C" {
#endif

void *vdec_thread(void *arg);

void *rga_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif
