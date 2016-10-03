#pragma once

#ifdef _ASSERT_H
#error 
#endif
#ifndef VULKANOMICS_DEBUG
#define NDEBUG
#endif
#include <assert.h>

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdio>

typedef unsigned char byte;

std::string strf(char const * fmt, ...) noexcept;
char const * vk_result_to_str(VkResult);

#define srcthrow(fmt, ...) throw vk::exception(strf("VULKANOMICS ERROR (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, strf(fmt, ##__VA_ARGS__).c_str()))

#ifdef VULKANOMICS_DEBUG
#define srcprintf_debug(fmt, ...) printf("%s\n", strf("VULKANOMICS DEBUG (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, strf(fmt, ##__VA_ARGS__).c_str()).c_str())
#else
#define srcprintf_debug(fmt, ...)
#endif

#define throw_fatal throw -1

#ifdef VULKANOMICS_DEBUG
#define VULKANOMICS_VK_DEBUG
#endif

#include "vulkanomics.hpp"
extern VkInstance vk_instance;

static thread_local VkResult vk_res;
#define VKR(call) vk_res = call; if (vk_res != VK_SUCCESS) srcthrow("\"%s\" unsuccessful: (%s)", #call, vk_result_to_str(vk_res));
