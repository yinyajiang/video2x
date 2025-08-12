#include "imgconversions.h"

#include <cstddef>
#include <cstdio>
#include <memory>

namespace image2x {

ncnn::Mat bgr24data_to_ncnn_mat(BGR24Data avdata) {
    // Allocate a new ncnn::Mat and copy the data
    ncnn::Mat ncnn_image = ncnn::Mat(avdata.width, avdata.height, static_cast<size_t>(3), 3);

    // Manually copy the pixel data from AVFrame to the new ncnn::Mat
    const uint8_t* src_data = avdata.framedata;
    for (int y = 0; y < avdata.height; y++) {
        uint8_t* dst_row = ncnn_image.row<uint8_t>(y);
        const uint8_t* src_row = src_data + y * avdata.width * 3;
        // Copy 3 channels (BGR) per pixel
        memcpy(dst_row, src_row, static_cast<size_t>(avdata.width) * 3);
    }

    return ncnn_image;
}

BGR24Data ncnn_mat_to_bgr24data(const ncnn::Mat& mat) {
    BGR24Data ret;
    memset(&ret, 0, sizeof(BGR24Data));

    ret.framedata = (uint8_t *)malloc(static_cast<size_t>(mat.w * mat.h * 3));
    ret.width = mat.w;
    ret.height = mat.h;
   
    // Copy the pixel data from ncnn::Mat to the BGR AVFrame
    for (int y = 0; y < mat.h; y++) {
        uint8_t* dst_row = ret.framedata + y * mat.w * 3;
        const uint8_t* src_row = mat.row<const uint8_t>(y);
        // Copy 3 channels (BGR) per pixel
        memcpy(dst_row, src_row, static_cast<size_t>(mat.w) * 3);
    }

    return ret;
}

void free_bgr24framedata(void* framedata){
    if(framedata){
        free(framedata);
    }
}

}
