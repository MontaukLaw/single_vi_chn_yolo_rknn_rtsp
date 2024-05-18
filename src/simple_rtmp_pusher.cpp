#include "simple_rtmp_pusher.h"
#include "user_define.h"

static AVFormatContext *ifmt_ctx = NULL;
static AVFormatContext *ofmt_ctx = NULL;
static AVOutputFormat *ofmt = NULL;
static int64_t start_time = 0;
static int64_t frame_index = 0;
static AVStream *in_stream, *out_stream;

static AVPacket pkt;
static int first_pack = 0;

void destropy_ctx(void)
{
    avformat_close_input(&ifmt_ctx);
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
    {
        avio_close(ofmt_ctx->pb);
    }
    avformat_free_context(ofmt_ctx);
}

void init_rtmp_connection(void)
{

    // AVPacket pkt;
    const char *in_filename, *out_filename;
    int ret, i;
    int videoindex = -1;

    // in_filename = "test10.h264";
    in_filename = "sample.h264";
    out_filename = "rtmp://43.139.145.196/live/livestream?secret=e8c13b9687ec47f989d80f08b763fdbf";
    // printf("%s\n", in_filename);

    av_register_all();

    printf(">> %s\n", out_filename);
    // Network
    avformat_network_init();

    printf("avformat_open_input\n");

    // Input
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0)
    {
        printf("Could not open input file.");
        destropy_ctx();
        return;
    }

    printf("avformat_find_stream_info\n");

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
    {
        printf("Failed to retrieve input stream information");
        destropy_ctx();
        return;
    }

    printf("fuck\n");

    if (ifmt_ctx == nullptr)
    {
        printf("ifmt_ctx is null\n");
        return;
    }

    printf("ifmt_ctx->nb_streams: %d\n", ifmt_ctx->nb_streams);

    for (i = 0; i < ifmt_ctx->nb_streams; i++)
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
            break;
        }

    printf("dump data\n");

    // Output
    avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_filename); // RTMP
    // avformat_alloc_output_context2(&ofmt_ctx, NULL, "mpegts", out_filename);//UDP

    if (!ofmt_ctx)
    {
        printf("Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        destropy_ctx();
        return;
    }

    printf("ofmt_ctx created\n");

    ofmt = ofmt_ctx->oformat;
    for (i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        // Create output AVStream according to input AVStream
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
        if (!out_stream)
        {
            printf("Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            destropy_ctx();
            return;
        }
        // Copy the settings of AVCodecContext
        ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if (ret < 0)
        {
            printf("Failed to copy context from input to output stream codec context\n");
            destropy_ctx();
            return;
        }
        out_stream->codec->codec_tag = 0;
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; // CODEC_FLAG_GLOBAL_HEADER;
    }
    // Dump Format------------------
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    // Open output URL
    if (!(ofmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            printf("Could not open output URL '%s'", out_filename);
            destropy_ctx();
            return;
        }
    }

    printf("send head\n");

    if (first_pack == 0)
    {
        ret = av_read_frame(ifmt_ctx, &pkt);
        first_pack = 1;
        if (ret < 0)
        {
            printf("av_read_frame failed\n");
            return;
        }
    }

    ifmt_ctx->streams[0]->time_base.num = 1;
    ifmt_ctx->streams[0]->time_base.den = 1000000;

    in_stream = ifmt_ctx->streams[pkt.stream_index];
    out_stream = ofmt_ctx->streams[pkt.stream_index];

    // Stream #0:0: Video: h264 (High), yuv420p(progressive), 1920x1080, 30 fps, 30 tbr, 1200k tbn, 60 tbc
    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    // Write file header
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0)
    {
        printf("Error occurred when opening output URL\n");
        destropy_ctx();
        return;
    }
    printf("head sent \n");

    // printf("start_time %ld\n", start_time);
}

void set_start_time(void)
{
    start_time = av_gettime();
}

void send_to_rtmp_server_testing(uint8_t *h264_data, int data_len, int64_t time_stamp)
{
    int ret = 0;

    // Get an AVPacket
    pkt.data = h264_data;
    pkt.size = data_len;

    // 重新给ifmt_ctx->streams[0]->time_base赋值
    // time_base: num: 1 len: 1000000
    // ifmt_ctx->streams[0]->time_base.num = 1;
    // ifmt_ctx->streams[0]->time_base.den = 1000000;

    // 由fps决定的, 40ms一帧, 由于现在的时基是毫秒，所以是40
    // int64_t calc_duration = 40;

    // pts是累计的帧数*帧间隔
    // pts以ｕｓ为单位
    pkt.pts = time_stamp / 1000; // (double)(frame_index * calc_duration);
    pkt.dts = pkt.pts;
    pkt.duration = 40;
    pkt.pos = -1;
    // 每个pts隔间是40000
    printf("pkt.pts:%lld\n", pkt.pts);
    // printf("pkt.pts:%lld duration:%lld\n", pkt.pts, pkt.duration);

    frame_index++;

    ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
    if (ret < 0)
    {
        printf("Error muxing packet\n");
        return;
    }

    av_free_packet(&pkt);
}
void send_to_rtmp_server_testedd(uint8_t *h264_data, int data_len)
{
    int ret = 0;
    AVStream *in_stream, *out_stream;
    if (first_pack == 0)
    {
        ret = av_read_frame(ifmt_ctx, &pkt);
        first_pack = 1;
        if (ret < 0)
        {
            printf("av_read_frame failed\n");
            return;
        }
    }

    // Get an AVPacket
    pkt.data = h264_data;
    pkt.size = data_len;
    // FIX No PTS (Example: Raw H.264)
    // Simple Write PTS
    // Write PTS
    // time_base: num: 1 len: 1200000
    AVRational time_base1 = ifmt_ctx->streams[0]->time_base;
    printf("time_base: num: %d len: %d\n", time_base1.num, time_base1.den);

    // Duration between 2 frames (us)
    // 25fps, 所以每帧的时间是 1/25 = 0.04s = 40ms
    int64_t calc_duration = AV_TIME_BASE / VIDEO_FPS;
    printf("Calc_duration:%lld\n", calc_duration);
    // Parameters
    // pts 就是累计的帧数*帧间隔除以时间基
    pkt.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
    printf("pkt.pts:%lld\n", pkt.pts);
    pkt.dts = pkt.pts;
    pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);

    // Important:Delay
    AVRational time_base = ifmt_ctx->streams[0]->time_base;
    AVRational time_base_q = {1, AV_TIME_BASE};
    printf("time_base: num: %d len: %d\n", time_base.num, time_base.den);
    printf("time_base_q: num: %d len: %d\n", time_base_q.num, time_base_q.den);

    int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
    int64_t now_time = av_gettime() - start_time;
    printf("pts_time:%lld , now_time :%lld \n", pts_time, now_time);
    if (pts_time > now_time)
    {
        printf(">>>>>>>>>>>>>>>>>>>>>>>>\n");
        av_usleep(pts_time - now_time);
    }

    in_stream = ifmt_ctx->streams[pkt.stream_index];
    out_stream = ofmt_ctx->streams[pkt.stream_index];
    /* copy packet */
    // Convert PTS/DTS
    pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
    pkt.pos = -1;
    printf(">> pkt.pts:%lld\n", pkt.pts);
    printf(">> pkt.dts:%lld\n", pkt.dts);
    printf(">> pkt.duration:%lld\n", pkt.duration);
    printf(">> pkt.pos:%lld\n", pkt.pos);
    // Print to Screen
    printf("Send %8d video frames to output URL\n", frame_index);
    frame_index++;

    for (int j = 0; j < 10; j++)
    {
        // printf("%02x ", pkt.data[j]);
    }
    // printf("\n");
    ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
    if (ret < 0)
    {
        printf("Error muxing packet\n");
        return;
    }

    av_free_packet(&pkt);
}
