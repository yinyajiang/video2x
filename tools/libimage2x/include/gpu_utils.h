#pragma once


#include <vector>
#include <string>

namespace image2x {

struct VKGPUInfo {
    std::string name;
    int index;
    std::string typeName;
    int type;
};

int get_gpu_count();
VKGPUInfo get_gpu_info(int index);
int get_default_gpu();
}