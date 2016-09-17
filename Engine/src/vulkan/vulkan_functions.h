#ifdef USE_RENDER_VULKAN
#ifndef VULKAN_FUNCTIONS_H
#define VULKAN_FUNCTIONS_H
#pragma once
#include "vulkan.h"

namespace HelloEngine
{

#define VK_EXPORTED_FUNCTION( fun ) extern PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION( fun) extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;

#include "vk_functions.inl"

} 
#endif
#endif

