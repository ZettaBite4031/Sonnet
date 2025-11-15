#pragma once
#ifndef SONNET_API
#if defined(_WIN32) || defined(__CYGWIN__)
#define SONNET_PLATFORM_WINDOWS 1
#else
#define SONNET_PLATFORM_WINDOWS 0
#endif
#if SONNET_PLATFORM_WINDOWS
#if defined(SONNET_BUILD_SHARED)
#define SONNET_API __desclspec(dllexport)
#elif definedS(SONNET_SHARED)
#define SONNET_API __desclspec(dllimport)
#endif
#else
#if defined(SONNET_BUILD_SHARED) || defined(SONNET_SHARED)
#if __GNUC__ >= 4
#define SONNET_API __attribute__((visibility("default")))
#else
#define SONNET_API
#endif // __GNUC__
#else
#define SONNET_API
#endif // SONNET_BUILD_SHARED || SONNET_SHARED
#endif // SONNET_PLATFORM_WINDOWS
#endif // SONNET_API

