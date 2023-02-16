#include "comm.h"
#include "postprocess.h"
#include "rknn_model.h"
#include "sample_common.h"
#include "rknn_funcs.h"
#include "yolo.h"

static void write_rgb_file(void *data) {
    char saveFilePath[128];
    memset(saveFilePath, 0, sizeof(saveFilePath));

    static int counter = 0;
    counter++;
    sprintf(saveFilePath, "/tmp/rga_%d.rgb", counter);

    FILE *save_file = fopen(saveFilePath, "w");
    if (!save_file) {
        printf("ERROR: Open %s failed!\n", saveFilePath);
    }

    fwrite(data, 1, YOLO_INPUT_SIZE, save_file);
    fclose(save_file);
}
