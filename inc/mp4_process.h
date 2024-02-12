#ifndef _MP4_PROCESS_H_
#define _MP4_PROCESS_H_

#include "mp4v2/mp4v2.h"
#include "pthread.h"

#include "rkmedia_api.h"
#include "rkmedia_venc.h"
#include "sys/time.h"
#include "comm.h"

#ifdef __cplusplus
extern "C"
{
#endif

    RK_S32 record_mp4(MEDIA_BUFFER mb);
    void close_mp4_file_saving(void);

#ifdef __cplusplus
}
#endif

#endif
