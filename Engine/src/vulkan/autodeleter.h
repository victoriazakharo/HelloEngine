#ifndef AUTODELETER_H
#define AUTODELETER_H
#pragma once

#ifdef USE_RENDER_VULKAN
#include "vulkan_functions.h"

namespace HelloEngine
{
	template<class T, class F>
	class AutoDeleter {
	public:
		AutoDeleter() :
			object(VK_NULL_HANDLE),
			deleter(nullptr),
			device(nullptr) {
		}

		AutoDeleter(T object, F deleter, VkDevice device) :
			object(object),
			deleter(deleter),
			device(device) {
		}

		AutoDeleter(AutoDeleter&& other) {
			*this = std::move(other);
		}

		~AutoDeleter() {
			if ((object != VK_NULL_HANDLE) && (deleter != nullptr) && device != nullptr) {
				deleter(device, object, nullptr);
			}
		}

		AutoDeleter& operator=(AutoDeleter&& other) {
			if (this != &other) {
				object = other.object;
				deleter = other.deleter;
				device = other.device;
				other.object = VK_NULL_HANDLE;
			}
			return *this;
		}

		T Get() {
			return object;
		}

		bool operator !() const {
			return object == VK_NULL_HANDLE;
		}
	private:
		AutoDeleter(const AutoDeleter&);
		T         object;
		F         deleter;
		VkDevice  device;
	};
}
#endif
#endif