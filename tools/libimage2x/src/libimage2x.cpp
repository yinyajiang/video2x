#include "imgconversions.h"
#include "imgfilter_realcugan.h"
#include "imgfilter_realesrgan.h"
#include <string>
#include "gpu_utils.h"
#include <string.h>
#include "fileutils.h"
#include <iostream>
#include <cassert>

image2x::IMGFilter* newIMGFilterRealcugan(ProcessorConfig config){
    auto model = config.scale <= 3 ? STR("models-pro") : STR("models-se");
    return new image2x::IMGFilterRealcugan(
            image2x::utf8_to_string_type(config.utf8ModelDir),
            config.vulkan_device_index,
            config.scale,
            model
    );
}

image2x::IMGFilter* newIMGFilterRealesrgan(ProcessorConfig config){
    auto model = STR("realesr-animevideov3");
    return new image2x::IMGFilterRealesrgan(
            image2x::utf8_to_string_type(config.utf8ModelDir),
            config.vulkan_device_index,
            config.scale,
            model
    );
}

void* create_image_processor(ProcessorConfig config){
 //anime、general、noise、sharpen
 assert(config.scale >= 2 && config.scale <= 4);
 if(config.vulkan_device_index <= 0){
     if(image2x::get_gpu_count() != 0){
         config.vulkan_device_index = image2x::get_default_gpu();
     }else{
         config.vulkan_device_index = -1;
     }
 }
 image2x::IMGFilter* filter = nullptr;
 if(std::string(config.name) == "anime"){
     filter = newIMGFilterRealcugan(config);
 }else if(std::string(config.name) == "noise"){
     filter = newIMGFilterRealesrgan(config);
 }else if(std::string(config.name) == "sharpen"){
     filter = newIMGFilterRealesrgan(config);
 }else{ //"general"
     filter = newIMGFilterRealcugan(config);
 }
 if(0 != filter->init()){
     std::cout << "Failed to initialize image processor" << std::endl;
     return nullptr;
 }
 return filter;
}

void free_image_processor(void* processor){
    if (processor){
        image2x::IMGFilter* p = static_cast<image2x::IMGFilter*>(processor);
        delete p;
    }
}

BGR24Data process_image(void* processor, BGR24Data avdata){
    image2x::IMGFilter* p = static_cast<image2x::IMGFilter*>(processor);
    return p->filter(avdata);
}

void free_framedata(uint8_t* framedata){    
    image2x::free_bgr24framedata(framedata);
}

int get_vkdev_count(){
    return image2x::get_gpu_count();
}

VKDeviceInfo get_vkdev_info(int index){
    auto count = image2x::get_gpu_count();
    assert(index >= 0 && index < count);
    auto info = image2x::get_gpu_info(index);
    VKDeviceInfo vkinfo;
    vkinfo.index = info.index + 1;
    strcpy(vkinfo.name, info.name.c_str());
    strcpy(vkinfo.type_name, info.typeName.c_str());
    return vkinfo;
}