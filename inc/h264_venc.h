#ifndef _H264_VENC_H_
#define _H264_VENC_H_

#include "rkmedia_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // void venc0_packet_cb(MEDIA_BUFFER mb);

    // int reg_venc_cb();

    void init_rtsp();

    void monitor_venc_packet_cb(MEDIA_BUFFER mb);

#ifdef __cplusplus
}
#endif

#endif
