#include "gpu_utils.h"

#include <iostream>
#include <vector>
#include <gpu.h>

namespace image2x {

    static VKGPUInfo ncnngpu_to_vkgpu(const ncnn::GpuInfo& gpu_info, int index) {
        VKGPUInfo dev;
        dev.name = gpu_info.device_name();
        dev.index = index;
        dev.type = gpu_info.type();

        // Map NCNN GPU type to readable string
        switch (gpu_info.type()) {
            case 0:  // Integrated GPU
                dev.typeName = "Integrated GPU";
                break;
            case 1:  // Discrete GPU
                dev.typeName = "Discrete GPU";
                break;
            case 2:  // Virtual GPU
                dev.typeName = "Virtual GPU";
                break;
            case 3:  // CPU
                dev.typeName = "CPU";
                break;
            default:
                dev.typeName = "Unknown";
                break;
        }
        return dev;
    }

    int get_gpu_count() {
        return ncnn::get_gpu_count();
    }

    VKGPUInfo get_gpu_info(int index) {
        const ncnn::GpuInfo& gpu_info = ncnn::get_gpu_info(index);
        return ncnngpu_to_vkgpu(gpu_info, index);
    }

    int get_default_gpu() {
        return ncnn::get_default_gpu_index();
    }

}
