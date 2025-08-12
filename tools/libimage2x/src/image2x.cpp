#include <iostream>
#if _WIN32
#include <Windows.h>
#include <cwchar>
#endif
#include <boost/program_options.hpp>
#define STB_IMAGE_IMPLEMENTATION
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

void save_png(std::filesystem::path path, const uint8_t* data, int w, int h);
void read_file(std::filesystem::path path, std::vector<uint8_t>& data);
void load_png(std::filesystem::path path, std::vector<uint8_t>& data, int& w, int& h);
namespace po = boost::program_options;

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char** argv) {
#endif
    image2x::StringType pngfile, pngrgb24, output, modelDir;
    std::string processor;
    int pngrgb24w,pngrgb24h;
    int vk_device_index;

    po::options_description all_opts("General options");
    all_opts.add_options()
         ("list-devices,l", "List the available Vulkan devices (GPUs)")
         ("png,i", PO_STR_VALUE<image2x::StringType>(&pngfile),
                "Input pngfile file path")
         ("pngrgb24,b", PO_STR_VALUE<image2x::StringType>(&pngrgb24),
                "Input bin file path")
         ("pngrgb24w,w", PO_STR_VALUE<int>(&pngrgb24w),
                "Input bin file path")
         ("pngrgb24h,h", PO_STR_VALUE<int>(&pngrgb24h),
                "Input bin file path")
         ("modeldir,m", PO_STR_VALUE<image2x::StringType>(&modelDir),
                 "Model dir bin path")
         ("output,o", PO_STR_VALUE<image2x::StringType>(&output),
                "Output video file path")
         ("processor,p", PO_STR_VALUE<std::string>(&processor)->default_value("general"),
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
    if(!pngfile.empty()){
        load_png(pngfile, bgr24data, pngrgb24w, pngrgb24h);
        if(output.empty()){
            output = pngfile + STR(".result.png");
        }
    }else if(!pngrgb24.empty()){
        read_file(pngrgb24, bgr24data);
        save_png(pngrgb24 + STR(".png"), &bgr24data[0], pngrgb24w, pngrgb24h);
        if(output.empty()){
            output = pngrgb24 + STR(".result.png");
        }
    }else{
        assert(0);
    }

    ProcessorConfig config;
    auto dir = image2x::string_type_to_u8string(modelDir);
    config.utf8ModelDir = dir.c_str();
    config.vulkan_device_index = vk_device_index;
    config.name = processor.c_str();
    config.scale = 2;
    auto p = create_image_processor(config);
    assert(p != nullptr);
    BGR24Data avdata;
    avdata.width = pngrgb24w;
    avdata.height = pngrgb24h;
    avdata.framedata = &bgr24data[0];
    auto res = process_image(p, avdata);
    assert(res.framedata != nullptr);
    save_png(output, res.framedata, res.width, res.height);
    return 0;
}

void save_png(std::filesystem::path path, const uint8_t* data, int w, int h){
    assert(1 == stbi_write_png(path.string().c_str(), w, h, 3, data, 0));
}

void read_file(std::filesystem::path path, std::vector<uint8_t>& data){
    std::ifstream ifs(path, std::ios_base::binary | std::ios_base::in);
    assert(ifs.is_open());
    ifs.seekg(0, std::ios::end);
    data.resize(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    ifs.read(reinterpret_cast<char*>(data.data()), data.size());
    assert(ifs.gcount() == data.size());
}

void load_png(std::filesystem::path path, std::vector<uint8_t>& data, int& w, int& h){
    int c;
    std::vector<uint8_t> filedata;
    read_file(path, filedata);
    auto pixeldata = stbi_load_from_memory(&filedata[0], filedata.size(), &w, &h, &c, 0);
    assert(pixeldata);
    if (c == 1)
    {
        stbi_image_free(pixeldata);
        pixeldata = stbi_load_from_memory(&filedata[0], filedata.size(), &w, &h, &c, 3);
        c = 3;
    }
    else if (c == 2)
    {
        stbi_image_free(pixeldata);
        pixeldata = stbi_load_from_memory(&filedata[0], filedata.size(), &w, &h, &c, 3);
        c = 3;
    }
    data.resize(w * h * c);
    memcpy(&data[0], pixeldata, data.size());
    stbi_image_free(pixeldata);
}