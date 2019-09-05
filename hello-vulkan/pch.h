#ifndef __PCH__
#define __PCH__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <vector>
#include <array>
#include <sstream>
#include <fstream>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_win32.h>
#include <GLTFSDK/GLTF.h>
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

//‚æ‚­Žg‚¢‚»‚¤‚È‚â‚Â‚ð’è‹`
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float float32;
typedef double float64;
typedef wchar_t wchar;

#endif//__PCH__
