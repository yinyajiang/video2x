#include <iostream>
#if _WIN32
#include <Windows.h>
#include <cwchar>
#endif
#include <boost/program_options.hpp>
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "fileutils.h"
#include "libimage2x.h"
#include <fstream>


#ifdef _WIN32
#define BOOST_PROGRAM_OPTIONS_WCHAR_T
#define PO_STR_VALUE po::wvalue
#else
#define PO_STR_VALUE po::value
#endif

int save_png(std::filesystem::path path, const uint8_t* data, int w, int h);
int read_file(std::filesystem::path path, std::vector<uint8_t>& data);
namespace po = boost::program_options;

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char** argv) {
#endif
    image2x::StringType pngfile, inputrgb24, output, processor;
    int inputrgb24w,inputrgb24h;
    int vk_device_index;

    po::options_description all_opts("General options");
    all_opts.add_options()
        ("list-devices,l", "List the available Vulkan devices (GPUs)")
        // General Processing Options
        ("png,i", PO_STR_VALUE<image2x::StringType>(&pngfile),
            "Input pngfile file path")
        ("inputrgb24,b", PO_STR_VALUE<image2x::StringType>(&inputrgb24),
             "Input bin file path")
        ("inputrgb24w,w", PO_STR_VALUE<int>(&inputrgb24w),
         "Input bin file path")
        ("inputrgb24h,h", PO_STR_VALUE<int>(&inputrgb24h),
         "Input bin file path")
        ("output,o", PO_STR_VALUE<image2x::StringType>(&output),
            "Output video file path")
        ("processor,p", PO_STR_VALUE<image2x::StringType>(&processor),
            "Processor to use (libplacebo, realesrgan, realcugan, rife)")
        ("device,d", po::value<int>(&vk_device_index)->default_value(0),
            "Vulkan device index (GPU ID)")
    ;
    po::variables_map vm;
#ifdef _WIN32
        po::store(po::wcommand_line_parser(argc, argv).options(all_opts).run(), vm);
#else
        po::store(po::command_line_parser(argc, argv).options(all_opts).run(), vm);
#endif
    po::notify(vm);
    if(vm.count("list-devices")){
        std::cout << "Listing Vulkan devices:" << std::endl;
        int count = get_vkdev_count();
        for(int i = 0;i < count; ++i){
           auto info = get_vkdev_info(i);
           std::cout << "Device " << i << ": " << info.name << std::endl;
           std::cout << "  Type: " << info.type_name << std::endl;
           std::cout << "  Index: " << info.index << std::endl;
        }
        return 0;
    }

    std::vector<uint8_t> bgr24data;
    if(!inputrgb24.empty()){
        auto res = read_file(inputrgb24, bgr24data);
        if(res != 0){
            return -1;
        }
        if(1 != save_png(inputrgb24 + STR(".png"), &bgr24data[0], inputrgb24w, inputrgb24h)){
            std::cerr << "Failed to save png file"<< std::endl;
            return 1;
        }
    }

    ProcessorConfig config;
    config.utf8ModelDir = "/Volumes/extern-usb/github/video2x/models";
    config.vulkan_device_index = 0;
    config.name = "general";
    config.scale = 2;
    auto p = create_image_processor(config);
    if(p == nullptr){
        std::cerr << "Failed to create image processor" << std::endl;
        return -1;
    }
    BGR24Data avdata;
    avdata.width = inputrgb24w;
    avdata.height = inputrgb24h;
    avdata.framedata = &bgr24data[0];
    auto res = process_image(p, avdata);
    if(res.framedata == nullptr){
        std::cerr << "Failed to process image" << std::endl;
        return -1;
    }
    if(1 != save_png(inputrgb24 + STR(".result.png"), res.framedata, res.width, res.height)){
        std::cerr << "Failed to save png file"<< std::endl;
        return 1;
    }
    return 0;
}


int save_png(std::filesystem::path path, const uint8_t* data, int w, int h){
    return stbi_write_png(path.string().c_str(), w, h, 3, data, 0);
}

int read_file(std::filesystem::path path, std::vector<uint8_t>& data){
    std::ifstream ifs(path, std::ios_base::binary | std::ios_base::in);
    if(!ifs.is_open()){
        std::cerr << "Failed to open file: " << path << std::endl;
        return -1;
    }
    ifs.seekg(0, std::ios::end);
    data.resize(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    ifs.read(reinterpret_cast<char*>(data.data()), data.size());
    if(ifs.gcount() != data.size()){
        std::cerr << "Failed to read file: " << path << std::endl;
        return -1;
    }
    return 0;
}