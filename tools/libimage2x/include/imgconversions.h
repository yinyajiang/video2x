#pragma once

#include <mat.h>
#include "libimage2x.h"
namespace image2x {

ncnn::Mat bgr24data_to_ncnn_mat(BGR24Data avdata);
BGR24Data ncnn_mat_to_bgr24data(const ncnn::Mat& mat);

void free_bgr24framedata(void* framedata);

}  // namespace video2x
