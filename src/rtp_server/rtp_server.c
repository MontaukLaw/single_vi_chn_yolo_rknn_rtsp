//
// Created by Liming Shao on 2018/5/10.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Utils.h"
#include "RTPEnc.h"
#include "Network.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_BUFFER_SIZE (1024 * 8)

// static FILE *gInputH264File = NULL;
typedef unsigned char uint8;
enum
{
    H264_NAL_SLICE = 1,
    H264_NAL_DPA = 2,
    H264_NAL_DPB = 3,
    H264_NAL_DPC = 4,
    H264_NAL_IDR = 5,
    H264_NAL_SEI = 6,
    H264_NAL_SPS = 7,
    H264_NAL_PPS = 8,
    H264_NAL_AUD = 9,
    H264_NAL_END_SEQUENCE = 10,
    H264_NAL_END_STREAM = 11,
    H264_NAL_FILLER_DATA = 12,
    H264_NAL_SPS_EXT = 13,
    H264_NAL_AUXILIARY_SLICE = 19,
};

uint8 avc_h264_nalu_type(uint8 *e_buf, int len)
{
    if (len > 4 && e_buf[0] == 0 && e_buf[1] == 0 && e_buf[2] == 0 && e_buf[3] == 1)
    {
        return (e_buf[4] & 0x1f);
    }
    else if (len > 3 && e_buf[0] == 0 && e_buf[1] == 0 && e_buf[2] == 1)
    {
        return (e_buf[3] & 0x1f);
    }
    else if (len > 0)
    {
        return (e_buf[0] & 0x1f);
    }

    return 0;
}

void send_rtp_package(RTPMuxContext rtpMuxContext, UDPContext udpContext, uint8 *buff, int size)
{
    uint8 nalu = 0;
    int offset = 4, next_startcode;
    bool uaf_rtp_tx = true;
    while (uaf_rtp_tx)
    {
        // buff[offset++] = fgetc(gInputH264File);

        nalu = avc_h264_nalu_type(buff, offset);

        next_startcode = 0;

        while (!next_startcode)
        {

            // buff[offset++] = fgetc(gInputH264File);

            if (buff[offset - 4] == 0 && buff[offset - 3] == 0 &&
                buff[offset - 2] == 0 && buff[offset - 1] == 1)
            {
                // 发现是H264_NAL_SLICE或者H264_NAL_IDR帧
                if (nalu == H264_NAL_SLICE || nalu == H264_NAL_IDR)
                {
                    next_startcode = 1;
                    break;
                }
                else
                {
                    // buff[offset++] = fgetc(gInputH264File);
                    offset++;
                    nalu = avc_h264_nalu_type(buff + offset - 5, 5);
                }
            }

            if (offset >= size)
            {
                printf("%s, offset=%d, max_size=%d\r\n", __FUNCTION__, offset, size);
                break;
            }
        }

        if (next_startcode)
        {
            rtpSendH264HEVC(&rtpMuxContext, &udpContext, buff, offset - 4);
            memmove(buff, buff + offset - 4, 4);
            offset = 4;
        }
        else
        {
            rtpSendH264HEVC(&rtpMuxContext, &udpContext, buff, offset);
            break;
        }
    }
}

void check_nal_head(uint8 *buff, int size, RTPMuxContext *rtpMuxContext, UDPContext *udpContext)
{

    int offset = 0;
    int found_counter = 0;
    int offset_idx[3];
    memset(offset_idx, 0, sizeof(offset_idx));

    uint8 *buff_start = malloc(size);

    int seg_size = 0;

    while (offset < size)
    {
        if (buff[offset] == 0 && buff[offset + 1] == 0 && buff[offset + 2] == 0 && buff[offset + 3] == 1)
        {

            offset_idx[found_counter] = offset;

            // if (offset > 0)
            // {
            //     printf("Found NALU head at offset %d\n", offset);
            // }

            found_counter++;
        }
        offset++;
    }

    // for (int i = 0; i < found_counter; i++)
    // {
    //     printf("Found NALU head at offset %d\n", offset_idx[i]);
    // }

    printf(">>>>>>>>>>>>> total size:%d\n", size);
    rtpSendH264HEVC(rtpMuxContext, udpContext, buff, size);

    return;
    // 只有1个NALU
    if (found_counter == 1)
    {
        // memset(buff_start, 0xFF, size);
        // memcpy(buff_start, buff, seg_size);
        // rtpSendH264HEVC(rtpMuxContext, udpContext, buff_start, size);
        rtpSendH264HEVC(rtpMuxContext, udpContext, buff, size);

        printf(">>>>>>>>>>>>> only 1 seg size :%d\n", size);
    }
    else
    {
        memset(buff_start, 0xFF, size);
        seg_size = offset_idx[1];
        printf(">>>>>>>>>>>>> 1 seg size :%d\n", seg_size);
        memcpy(buff_start, buff, seg_size);
        rtpSendH264HEVC(rtpMuxContext, udpContext, buff_start, seg_size);

        memset(buff_start, 0xFF, size);
        seg_size = offset_idx[2];
        printf(">>>>>>>>>>>>> 2 seg size :%d\n", seg_size);
        memcpy(buff_start, buff + offset_idx[1], seg_size);
        rtpSendH264HEVC(rtpMuxContext, udpContext, buff_start, seg_size);

        memset(buff_start, 0xFF, size);
        seg_size = size - offset_idx[2] - offset_idx[1];
        printf(">>>>>>>>>>>>> 3 seg size :%d\n", seg_size);
        memcpy(buff_start, buff + offset_idx[2] + offset_idx[1], seg_size);
        rtpSendH264HEVC(rtpMuxContext, udpContext, buff_start, seg_size);

        rtpMuxContext->timestamp += (90000.0 / 25);
    }
}

static RTPMuxContext rtpMuxContext;
static UDPContext udpContext = {
    .dstIp = "192.168.1.242", // destination ip
    .dstPort = 1234           // destination port
};

void init_rtp()
{

    int res = udpInit(&udpContext);
    if (res)
    {
        printf("udpInit error.\n");
    }

    initRTPMuxContext(&rtpMuxContext);
}

int feed_rtp(uint8 *buff, int size)
{

    uint8 nalu = 0;
    int offset = 0, next_startcode;

    // if (buff[0] != 0 || buff[1] != 0 || buff[2] != 0 || buff[3] != 1)
    // {
    //     // free(buff);
    //     printf("Invalid h264 data.\n");
    //     return -1;
    // }

    // nalu = avc_h264_nalu_type(buff, offset);
    // if (next_startcode)
    // {
    //     rtpSendH264HEVC(&rtpMuxContext, &udpContext, buff, offset - 4);
    //     memmove(buff, buff + offset - 4, 4);
    //     offset = 4;
    // }
    // else
    // {
    // printf("-----------PACKAGE START---------------\n");
    rtpSendH264HEVC(&rtpMuxContext, &udpContext, buff, size);
    // check_nal_head(buff, size, &rtpMuxContext, &udpContext);
    rtpMuxContext.timestamp += (90000.0 / 25);

    // printf("-----------PACKAGE END---------------\n");

    // send_rtp_package(rtpMuxContext, udpContext, buff, size);

    // rtpSendH264HEVC(&rtpMuxContext, &udpContext, buff, size);
    //  return;
    //}
}
