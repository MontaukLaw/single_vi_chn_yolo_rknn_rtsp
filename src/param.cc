#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include "monitor.h"

g_params gParams;

std::map<std::string, std::string> readConfig(const std::string &filename)
{
    std::map<std::string, std::string> config;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line))
    {
        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '='))
        {
            std::string value;
            if (std::getline(is_line, value))
            {
                config[key] = value;
            }
        }
    }

    // 使用配置项
    gParams.save_mp4 = config["save_mp4"] == "true";
    gParams.mp4_length_sec = std::stoi(config["mp4_length"]);
    gParams.draw_distance = config["draw_distance"] == "true";
    gParams.point_x = std::stoi(config["point_x"]);
    gParams.point_y = std::stoi(config["point_y"]);
    gParams.chinese_font_path = config["chinese_font_path"];
    gParams.mp4_folder = config["mp4_folder"];
    gParams.water_mark_font_size = std::stoi(config["water_mark_font_size"]);
    gParams.jpeg_folder = config["jpeg_folder"];
    gParams.jpeg_quality = std::stoi(config["jpeg_quality"]);
    gParams.jpeg_width = std::stoi(config["jpeg_width"]);
    gParams.jpeg_height = std::stoi(config["jpeg_height"]);
    gParams.jpeg_interval = std::stoi(config["jpeg_interval"]);
    gParams.water_mark_y_gap = std::stoi(config["water_mark_y_gap"]);

    int i = 0;
    for (i = 0; i < WATER_MARK_TEXT_LINES; i++)
    {
        // water_mark_wn_line1_text
        // water_mark_en_start_x
        gParams.wn_water_mark[i].text = config["water_mark_wn_line" + std::to_string(i) + "_text"];
        gParams.wn_water_mark[i].start_x = std::stoi(config["water_mark_wn_start_x"]);
        gParams.wn_water_mark[i].start_y = std::stoi(config["water_mark_wn_start_y"]) + i * gParams.water_mark_y_gap;
    }

    for (i = 0; i < WATER_MARK_TEXT_LINES; i++)
    {
        // water_mark_wn_line1_text
        gParams.ws_water_mark[i].text = config["water_mark_ws_line" + std::to_string(i) + "_text"];
        gParams.ws_water_mark[i].start_x = std::stoi(config["water_mark_ws_start_x"]);
        gParams.ws_water_mark[i].start_y = std::stoi(config["water_mark_ws_start_y"]) + i * gParams.water_mark_y_gap;
    }

    for (i = 0; i < WATER_MARK_TEXT_LINES; i++)
    {
        // water_mark_es_line0_text
        gParams.es_water_mark[i].text = config["water_mark_es_line" + std::to_string(i) + "_text"];
        gParams.es_water_mark[i].start_x = std::stoi(config["water_mark_es_start_x"]);
        gParams.es_water_mark[i].start_y = std::stoi(config["water_mark_es_start_y"]) + i * gParams.water_mark_y_gap;
    }

    for (i = 0; i < WATER_MARK_TEXT_LINES; i++)
    {
        // water_mark_wn_line1_text
        gParams.en_water_mark[i].text = config["water_mark_en_line" + std::to_string(i) + "_text"];
        gParams.en_water_mark[i].start_x = std::stoi(config["water_mark_en_start_x"]);
        gParams.en_water_mark[i].start_y = std::stoi(config["water_mark_en_start_y"]) + i * gParams.water_mark_y_gap;
    }

    printf("Save MP4: %d\n", gParams.save_mp4);
    printf("MP4 Length: %d\n", gParams.mp4_length_sec);
    printf("Draw Distance: %d\n", gParams.draw_distance);
    printf("Point X: %d\n", gParams.point_x);
    printf("Point Y: %d\n", gParams.point_y);
    // printf("Water Mark Start X: %d\n", gParams.water_mark_start_x);
    // printf("Water Mark Start Y: %d\n", gParams.water_mark_start_y);
    // printf("Water Mark Text: %s\n", gParams.water_mark_text.c_str());
    printf("Chinese Font Path: %s\n", gParams.chinese_font_path.c_str());
    printf("MP4 Folder: %s\n", gParams.mp4_folder.c_str());
    printf("Water Mark Font Size: %d\n", gParams.water_mark_font_size);
    printf("JPEG Folder: %s\n", gParams.jpeg_folder.c_str());
    printf("JPEG Quality: %d\n", gParams.jpeg_quality);
    printf("JPEG Width: %d\n", gParams.jpeg_width);
    printf("JPEG Height: %d\n", gParams.jpeg_height);
    printf("JPEG Interval: %d\n", gParams.jpeg_interval);

    for (i = 0; i < 3; i++)
    {
        printf("WS Water Mark Start X: %d\n", gParams.ws_water_mark[i].start_x);
        printf("WS Water Mark Start Y: %d\n", gParams.ws_water_mark[i].start_y);
        printf("WS Water Mark Text: %s\n", gParams.ws_water_mark[i].text.c_str());
    }

    for (i = 0; i < 3; i++)
    {
        printf("ES Water Mark Start X: %d\n", gParams.es_water_mark[i].start_x);
        printf("ES Water Mark Start Y: %d\n", gParams.es_water_mark[i].start_y);
        printf("ES Water Mark Text: %s\n", gParams.es_water_mark[i].text.c_str());
    }

    for (i = 0; i < 3; i++)
    {
        printf("WN Water Mark Start X: %d\n", gParams.wn_water_mark[i].start_x);
        printf("WN Water Mark Start Y: %d\n", gParams.wn_water_mark[i].start_y);
        printf("WN Water Mark Text: %s\n", gParams.wn_water_mark[i].text.c_str());
    }

    for (i = 0; i < 3; i++)
    {
        printf("EN Water Mark Start X: %d\n", gParams.en_water_mark[i].start_x);
        printf("EN Water Mark Start Y: %d\n", gParams.en_water_mark[i].start_y);
        printf("EN Water Mark Text: %s\n", gParams.en_water_mark[i].text.c_str());
    }

    return config;
}