#include "imgfilter_realcugan.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include "imgconversions.h"
#include "fileutils.h"

namespace image2x {

IMGFilterRealcugan::IMGFilterRealcugan(
    const StringType model_dir,
    int gpuid,
    int scaling_factor,
    const StringType model_name,
    int noise_level,
    bool tta_mode,
    int num_threads,
    int syncgap
)
    : realcugan_(nullptr),
      gpuid_(gpuid),
      tta_mode_(tta_mode),
      scaling_factor_(scaling_factor),
      noise_level_(noise_level),
      num_threads_(num_threads),
      syncgap_(syncgap),
      model_dir_(std::move(model_dir)),
      model_name_(std::move(model_name)) {}

IMGFilterRealcugan::~IMGFilterRealcugan() {
    delete realcugan_;
    realcugan_ = nullptr;
}

int IMGFilterRealcugan::init() {
    // Construct the model paths using std::filesystem
    std::filesystem::path model_param_path;
    std::filesystem::path model_bin_path;

    StringType model_base_name =
        STR("up") + to_string_type(scaling_factor_) + STR("x-");

    switch (noise_level_) {
        case -1:
            model_base_name += STR("conservative");
            break;
        case 0:
            model_base_name += STR("no-denoise");
            break;
        default:
            model_base_name += STR("denoise") + to_string_type(noise_level_) + STR("x");
            break;
    }

    StringType param_file_name = model_base_name + STR(".param");
    StringType bin_file_name = model_base_name + STR(".bin");

    // Find the model paths by model name if provided
    model_param_path =
        std::filesystem::path(model_dir_) / STR("realcugan") / model_name_ / param_file_name;
    model_bin_path =
        std::filesystem::path(model_dir_) / STR("realcugan") / model_name_ / bin_file_name;


    // Check if the model files exist
    std::error_code e;
    if (!std::filesystem::exists(model_param_path, e)) {
        std::cout << "Real-CUGAN model param file not found: " << model_param_path.u8string() << std::endl;
        return -1;
    }
    if (!std::filesystem::exists(model_bin_path, e)) {
        std::cout << "Real-CUGAN model bin file not found: " <<  model_bin_path.u8string() << std::endl;
        return -1;
    }

    // Create a new Real-CUGAN instance
    realcugan_ = new RealCUGAN(gpuid_, tta_mode_, num_threads_);

    // Load the model
    if (realcugan_->load(model_param_path, model_bin_path) != 0) {
        std::cout << "Failed to load Real-CUGAN model" << std::endl;
        return -1;
    }

    // Set syncgap to 0 for models-nose
    if (model_name_.find(STR("models-nose")) != StringType::npos) {
        syncgap_ = 0;
    }

    // Set realcugan parameters
    realcugan_->scale = scaling_factor_;
    realcugan_->noise = noise_level_;
    realcugan_->prepadding = 10;

    // Set prepadding based on scaling factor
    if (scaling_factor_ == 2) {
        realcugan_->prepadding = 18;
    }
    if (scaling_factor_ == 3) {
        realcugan_->prepadding = 14;
    }
    if (scaling_factor_ == 4) {
        realcugan_->prepadding = 19;
    }

    // Calculate tilesize based on GPU heap budget
    ncnn::VulkanDevice * vkdev = nullptr;
    if(gpuid_ >= 0){
        vkdev = ncnn::get_gpu_device(gpuid_);
    }
    if(vkdev){
        uint32_t heap_budget = vkdev->get_heap_budget();
        if (scaling_factor_ == 2) {
            if (heap_budget > 1300) {
                realcugan_->tilesize = 400;
            } else if (heap_budget > 800) {
                realcugan_->tilesize = 300;
            } else if (heap_budget > 400) {
                realcugan_->tilesize = 200;
            } else if (heap_budget > 200) {
                realcugan_->tilesize = 100;
            } else {
                realcugan_->tilesize = 32;
            }
        }
        if (scaling_factor_ == 3) {
            if (heap_budget > 3300) {
                realcugan_->tilesize = 400;
            } else if (heap_budget > 1900) {
                realcugan_->tilesize = 300;
            } else if (heap_budget > 950) {
                realcugan_->tilesize = 200;
            } else if (heap_budget > 320) {
                realcugan_->tilesize = 100;
            } else {
                realcugan_->tilesize = 32;
            }
        }
        if (scaling_factor_ == 4) {
            if (heap_budget > 1690) {
                realcugan_->tilesize = 400;
            } else if (heap_budget > 980) {
                realcugan_->tilesize = 300;
            } else if (heap_budget > 530) {
                realcugan_->tilesize = 200;
            } else if (heap_budget > 240) {
                realcugan_->tilesize = 100;
            } else {
                realcugan_->tilesize = 32;
            }
        }
    }else{
        realcugan_->tilesize = 400;
    }
    return 0;
}

BGR24Data IMGFilterRealcugan::filter(BGR24Data avdata) {
    // Convert the input frame to RGB24
    ncnn::Mat in_mat = bgr24data_to_ncnn_mat(avdata);
    if (in_mat.empty()) {
        std::cout << "Failed to convert AVFrame to ncnn::Mat" << std::endl;
        return BGR24Data{};
    }

    // Allocate space for output ncnn::Mat
    int output_width = in_mat.w * realcugan_->scale;
    int output_height = in_mat.h * realcugan_->scale;
    ncnn::Mat out_mat = ncnn::Mat(output_width, output_height, static_cast<size_t>(3), 3);
    int res = 0;
    if(gpuid_ == -1){
        res = realcugan_->process_cpu(in_mat, out_mat);
    }else{
        res = realcugan_->process(in_mat, out_mat);
    }
    if (res != 0) {
        std::cout << "Real-CUGAN processing failed" << std::endl;
        return BGR24Data{};
    }

    return ncnn_mat_to_bgr24data(out_mat);
}

}
