#ifndef __MONITOR_H_
#define __MONITOR_H_
#include <map>
#include "opencv2/opencv.hpp"
#include "opencv2/freetype.hpp"


#ifdef __cplusplus
extern "C"
{
#endif

#include "comm.h"

    typedef struct water_mark_t
    {
        int start_x;
        int start_y;
        std::string text;
        int font_size;
    } water_mark;

    typedef struct g_params_t
    {
        bool save_mp4;
        int mp4_length_sec;
        bool draw_distance;
        int point_x;
        int point_y;
        std::string chinese_font_path;
        std::string mp4_folder;
        int water_mark_font_size;
        std::string jpeg_folder;
        int jpeg_quality;
        int jpeg_width;
        int jpeg_height;
        int jpeg_interval;
        int water_mark_y_gap;

        water_mark ws_water_mark[WATER_MARK_TEXT_LINES];
        water_mark es_water_mark[WATER_MARK_TEXT_LINES];
        water_mark wn_water_mark[WATER_MARK_TEXT_LINES];
        water_mark en_water_mark[WATER_MARK_TEXT_LINES];

        int clean_old_box_frames;
        int defog_level;
        bool fec_enable;
    } g_params;

    std::map<std::string, std::string> readConfig(const std::string &filename);

    void monitor_rga_packet_cb(MEDIA_BUFFER mb);

    void *observer_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif
