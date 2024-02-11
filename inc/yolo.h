#ifndef __YOLO_V5_H_
#define __YOLO_V5_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "comm.h"

    // #define OBJ_CLASS_NUM 80
    // #define PROP_BOX_SIZE (5 + OBJ_CLASS_NUM)
    // #define NMS_THRESHOLD 0.45
    // #define CONF_THRESHOLD 0.45 // 0.25

    int readLines(const char *fileName, char *lines[], int max_line);

    int compute_letter_box(LETTER_BOX *lb);

    int post_process(rknn_output *rk_outputs, MODEL_INFO *m, LETTER_BOX *lb, detect_result_group_t *group);

    int readFloats(const char *fileName, float *result, int max_line, int *valid_number);

    int post_process_640_v5(rknn_output *rk_outputs, MODEL_INFO *m, detect_result_group_t *group);

    int loadLabelName(const char *locationFilename, char *label[]);

    void set_conf(float f);

#ifdef __cplusplus
}
#endif

#endif //__YOLO_V5_H_
