#pragma once

#include "libimage2x_export.h"

extern "C" {

#pragma pack(push, 1)
struct BGR24Data {
    uint8_t* framedata;
    int width;
    int height;
};

struct ProcessorConfig{
    const char* name; //anime、general、denoise、sharpen
    int scale; //2, 3, 4
    int vulkan_device_index; // 0 == default
    const char* utf8ModelDir;
};

struct VKDeviceInfo{
    char name[260];
    int index;
    char type_name[260];
};

#pragma pack(pop)

LIBIMAGE2X_API void* create_image_processor(ProcessorConfig config);
LIBIMAGE2X_API void free_image_processor(void* processor);
LIBIMAGE2X_API BGR24Data process_image(void* processor, BGR24Data avdata);
LIBIMAGE2X_API void free_framedata(uint8_t* framedata);
LIBIMAGE2X_API int get_vkdev_count();
LIBIMAGE2X_API VKDeviceInfo get_vkdev_info(int index);

}

