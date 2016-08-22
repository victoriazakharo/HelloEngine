
#ifndef HELLO_ENGINE_API_H
#define HELLO_ENGINE_API_H

#ifdef HELLOENGINE_STATIC_DEFINE
#  define HELLO_ENGINE_API
#  define HELLOENGINE_NO_EXPORT
#else
#  ifndef HELLO_ENGINE_API
#    ifdef Engine_EXPORTS
        /* We are building this library */
#      define HELLO_ENGINE_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define HELLO_ENGINE_API __declspec(dllimport)
#    endif
#  endif

#  ifndef HELLOENGINE_NO_EXPORT
#    define HELLOENGINE_NO_EXPORT 
#  endif
#endif

#ifndef HELLOENGINE_DEPRECATED
#  define HELLOENGINE_DEPRECATED __declspec(deprecated)
#endif

#ifndef HELLOENGINE_DEPRECATED_EXPORT
#  define HELLOENGINE_DEPRECATED_EXPORT HELLO_ENGINE_API HELLOENGINE_DEPRECATED
#endif

#ifndef HELLOENGINE_DEPRECATED_NO_EXPORT
#  define HELLOENGINE_DEPRECATED_NO_EXPORT HELLOENGINE_NO_EXPORT HELLOENGINE_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define HELLOENGINE_NO_DEPRECATED
#endif

#endif
