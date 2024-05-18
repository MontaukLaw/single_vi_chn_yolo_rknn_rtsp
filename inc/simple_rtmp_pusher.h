#ifndef _SIMPLE_RTMP_PUSHER_H_
#define _SIMPLE_RTMP_PUSHER_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#ifdef __cplusplus
};
#endif

void init_rtmp_connection(void);

void send_to_rtmp_server(uint8_t *h264_data, int data_len);

void set_start_time(void);

#endif