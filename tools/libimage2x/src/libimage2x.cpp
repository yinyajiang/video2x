#include "imgconversions.h"
#include "imgfilter_realcugan.h"
#include "imgfilter_realesrgan.h"
#include <string>
#include "gpu_utils.h"
#include <string.h>
#include "fileutils.h"
#include <iostream>
#include <cassert>

image2x::IMGFilter* newIMGFilterRealcugan(ProcessorConfig config, int noise_level = -1, bool tta_mode = false){
    assert(noise_level <= 3);
    auto model = config.scale <= 3 ? STR("models-pro") : STR("models-se");
    if(config.scale <= 3 && noise_level >= 1){
        model = STR("models-se");
    }

    if(image2x::StringType(model) == STR("models-pro")){
        assert(config.scale <= 3);
        if (noise_level != -1 && noise_level != 0 && noise_level != 3) {
            noise_level = -1;
        }
    }else if(image2x::StringType(model) == STR("models-se")){
        if (noise_level != -1 && noise_level != 0 && noise_level != 3) {
            noise_level = -1;
        }
    }else{
        assert(0);
    }
    return new image2x::IMGFilterRealcugan(
            image2x::utf8_to_string_type(config.utf8ModelDir),
            config.vulkan_device_index,
            config.scale,
            model,
            noise_level,
            tta_mode
    );
}

image2x::IMGFilter* newIMGFilterRealesrgan(ProcessorConfig config, bool prefer_animevideovs, int noise_level = 0, bool tta_mode = false){
    assert(noise_level <= 3);
    auto model = config.scale < 4 ? STR("realesr-animevideov3") : STR("realesr-generalv3");
    if(prefer_animevideovs){
        model = STR("realesr-animevideov3");
    }

    if(config.scale < 4){
        model = STR("realesr-animevideov3");
    }
    if(noise_level > 0 && config.scale == 4){
        model = STR("realesr-generalv3");
    }
    if(noise_level > 0 && config.scale != 4){
        noise_level = 0;
    }
    return new image2x::IMGFilterRealesrgan(
            image2x::utf8_to_string_type(config.utf8ModelDir),
            config.vulkan_device_index,
            config.scale,
            model,
            noise_level,
            tta_mode
    );
}

static std::string g_default_model_dir = "";
void init_default_model_dir(){
    if(g_default_model_dir.empty()){
        g_default_model_dir = image2x::string_type_to_u8string(image2x::get_exe_directory()) + "/models";
    }
}

void* create_image_processor(ProcessorConfig config){
 //anime、general、denoise、sharpen
 assert(config.scale >= 2 && config.scale <= 4);
 if(config.vulkan_device_index == 0){
     if(image2x::get_gpu_count() != 0){
         config.vulkan_device_index = image2x::get_default_gpu();
     }else{
         config.vulkan_device_index = -1;
     }
 }

 if(config.utf8ModelDir == nullptr || config.utf8ModelDir[0] == '\0'){
    init_default_model_dir();
    config.utf8ModelDir = g_default_model_dir.c_str();
 }

 image2x::IMGFilter* filter = nullptr;
 if(std::string(config.name) == "anime"){
    filter = config.vulkan_device_index == -1 ? newIMGFilterRealcugan(config) : newIMGFilterRealesrgan(config, true);
 }else if(std::string(config.name) == "denoise"){
    filter = newIMGFilterRealcugan(config, 3, false);
 }else if(std::string(config.name) == "sharpen"){
    filter = newIMGFilterRealcugan(config, 3, false);
 }else{ //"general"
    filter = config.vulkan_device_index == -1 ? newIMGFilterRealcugan(config) : newIMGFilterRealesrgan(config, true);
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
    memset(vkinfo.name, 0, sizeof(vkinfo.name));
    memset(vkinfo.type_name, 0, sizeof(vkinfo.type_name));
    strcpy(vkinfo.name, info.name.c_str());
    strcpy(vkinfo.type_name, info.typeName.c_str());
    return vkinfo;
}