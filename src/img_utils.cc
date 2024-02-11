#include "comm.h"

void fill_image_data(void *inputData, void *modelData, int inputWidth, int inputHeight, int modelWidth, int modelHeight)
{
    unsigned char *src = static_cast<unsigned char *>(inputData);
    unsigned char *dest = static_cast<unsigned char *>(modelData);

    // 确保宽度相同
    if (inputWidth != modelWidth)
    {
        printf("Width not match\n");
        return;
    }

    // 计算垂直居中时的起始行（在目标图像中）
    int verticalPadding = (modelHeight - inputHeight) / 2;

    // 每个像素3个字节（RGB）
    int bytesPerPixel = 3;

    // 计算每行数据的字节数
    int rowBytes = inputWidth * bytesPerPixel;

    // 计算目标开始位置的指针
    unsigned char *destRowStart = dest + (verticalPadding * rowBytes);

    // 一次性复制整个图像数据块
    memcpy(destRowStart, src, inputHeight * rowBytes);
}
