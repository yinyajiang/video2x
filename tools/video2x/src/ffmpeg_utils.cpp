#include <vector>
#include <libvideo2x/libvideo2x.h>

std::vector<AVHWDeviceType> get_supported_hw_device_types() {
    std::vector<AVHWDeviceType> supported_types;
    auto prev = av_hwdevice_iterate_types(AV_HWDEVICE_TYPE_NONE);
    while(prev != AV_HWDEVICE_TYPE_NONE){
        const char* name = av_hwdevice_get_type_name(prev);
        bool supported = false;

        AVBufferRef* device_ctx = nullptr;
        av_log_set_level(AV_LOG_FATAL);
        int ret = av_hwdevice_ctx_create(&device_ctx, prev, nullptr, nullptr, 0);
        av_log_set_level(AV_LOG_INFO);

        if (ret >= 0 && device_ctx) {
            supported = true;
            av_buffer_unref(&device_ctx);
        }

        if(supported){
            supported_types.push_back(prev);
        }
        prev = av_hwdevice_iterate_types(prev);
    }
    return supported_types;
}