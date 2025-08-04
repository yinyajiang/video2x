#pragma once

#include <vulkan/vulkan.h>

int list_vulkan_devices();

int get_vulkan_device_prop(int vk_device_index, VkPhysicalDeviceProperties* dev_props);
