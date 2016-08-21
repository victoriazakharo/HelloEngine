#pragma once
#ifdef USE_PLATFORM_WIN32_KHR
#define HELLO_ENGINE_API __declspec(dllexport) 
#else
#define HELLO_ENGINE_API 
#endif