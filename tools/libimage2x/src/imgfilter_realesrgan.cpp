#include "imgfilter_realesrgan.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include "imgconversions.h"
#include "fileutils.h"


namespace image2x {

IMGFilterRealesrgan::IMGFilterRealesrgan(
    const StringType model_dir,
    int gpuid,
    int scaling_factor,
    const  StringType model_name,
    int noise_level,
    bool tta_mode
)
    : realesrgan_(nullptr),
      gpuid_(gpuid),
      tta_mode_(tta_mode),
      scaling_factor_(scaling_factor),
      noise_level_(noise_level),
      model_dir_(std::move(model_dir)),
      model_name_(std::move(model_name)) {}

IMGFilterRealesrgan::~IMGFilterRealesrgan() {
    delete realesrgan_;
    realesrgan_ = nullptr;
}

int IMGFilterRealesrgan::init() {
    // Construct the model paths using std::filesystem
    std::filesystem::path model_param_path;
    std::filesystem::path model_bin_path;

    StringType param_file_name = model_name_;
    StringType bin_file_name = model_name_;

    if (model_name_ == STR("realesr-generalv3") && noise_level_ > 0) {
        param_file_name += STR("-wdn");
        bin_file_name += STR("-wdn");
    }

    param_file_name += STR("-x") + to_string_type(scaling_factor_) + STR(".param");
    bin_file_name += STR("-x") + to_string_type(scaling_factor_) + STR(".bin");

    // Find the model paths by model name if provided
    model_param_path = std::filesystem::path(model_dir_) / STR("realesrgan") / param_file_name;
    model_bin_path = std::filesystem::path(model_dir_) / STR("realesrgan") / bin_file_name;


    // Check if the model files exist
    std::error_code e;
    if (!std::filesystem::exists(model_param_path, e)) {
        std::cout << "Real-ESRGAN model param file not found: " << model_param_path.u8string() << std::endl;
        return -1;
    }
    if (!std::filesystem::exists(model_bin_path, e)) {
        std::cout << "Real-ESRGAN model bin file not found: " << model_bin_path.u8string() << std::endl;
        return -1;
    }

    // Create a new Real-ESRGAN instance
    realesrgan_ = new RealESRGAN(gpuid_, tta_mode_);

    // Load the model
    if (realesrgan_->load(model_param_path, model_bin_path) != 0) {
        std::cout <<"Failed to load Real-ESRGAN model" << std::endl;
        return -1;
    }

    // Set Real-ESRGAN parameters
    realesrgan_->scale = scaling_factor_;
    realesrgan_->prepadding = 10;

    // Calculate tilesize based on GPU heap budget
    ncnn::VulkanDevice * vkdev = nullptr;
    if(gpuid_ >= 0){
        vkdev = ncnn::get_gpu_device(gpuid_);
    }
    if(vkdev){
        uint32_t heap_budget = ncnn::get_gpu_device(gpuid_)->get_heap_budget();
        if (heap_budget > 1900) {
            realesrgan_->tilesize = 200;
        } else if (heap_budget > 550) {
            realesrgan_->tilesize = 100;
        } else if (heap_budget > 190) {
            realesrgan_->tilesize = 64;
        } else {
            realesrgan_->tilesize = 32;
        }
    }else{
        realesrgan_->tilesize = 400;
    }

    return 0;
}

BGR24Data IMGFilterRealesrgan::filter(BGR24Data avdata) {
    // Convert the input frame to RGB24
    ncnn::Mat in_mat = bgr24data_to_ncnn_mat(avdata);
    if (in_mat.empty()) {
        std::cout << "Failed to convert AVFrame to ncnn::Mat" << std::endl;
        return BGR24Data{};
    }

    // Allocate space for output ncnn::Mat
    int output_width = in_mat.w * realesrgan_->scale;
    int output_height = in_mat.h * realesrgan_->scale;
    ncnn::Mat out_mat = ncnn::Mat(output_width, output_height, static_cast<size_t>(3), 3);

    auto res = realesrgan_->process(in_mat, out_mat);
    if (res != 0) {
        std::cout << "Real-ESRGAN processing failed" << std::endl;
        return BGR24Data{};
    }
    return ncnn_mat_to_bgr24data(out_mat);
}

}
