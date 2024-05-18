#include "h264_venc.h"
#include "rtsp_demo.h"
#include "user_define.h"
#include "comm.h"

static AVFormatContext *ifmt_ctx = NULL;
static AVFormatContext *ofmt_ctx = NULL;
static AVOutputFormat *ofmt = NULL;
static int64_t start_time = 0;
static int64_t frame_index = 0;

static void destropy_ctx(void)
{
    avformat_close_input(&ifmt_ctx);
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
    {
        avio_close(ofmt_ctx->pb);
    }
    avformat_free_context(ofmt_ctx);
}

int main(void)
{

    // AVPacket pkt;
    const char *in_filename, *out_filename;
    int ret, i;
    int videoindex = -1;

    // in_filename = "test10.h264";
    // in_filename = "sample.h264";
    in_filename = "safe_vese_and_hat.mp4";
    out_filename = "rtmp://43.139.145.196/live/livestream?secret=e8c13b9687ec47f989d80f08b763fdbf";
    // printf("%s\n", in_filename);

    av_register_all();

    printf(">> %s\n", out_filename);
    // Network
    avformat_network_init();

    // Input
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0)
    {
        printf("Could not open input file.");
        destropy_ctx();
        return -1;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
    {
        printf("Failed to retrieve input stream information");
        destropy_ctx();
          return -1;
    }

    if (ifmt_ctx == nullptr)
    {
        printf("ifmt_ctx is null\n");
         return -1;
    }

    videoindex = i;

    // Stream #0:0: Video: h264 (High), yuv420p(progressive), 1920x1080, 30 fps, 30 tbr, 1200k tbn, 60 tbc
    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    return 0;
}