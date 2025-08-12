#pragma once

#ifdef _WIN32
#ifdef LIBIMAGE2X_EXPORTS
#define LIBIMAGE2X_API __declspec(dllexport)
#else
#define LIBIMAGE2X_API __declspec(dllimport)
#endif
#else
#define LIBIMAGE2X_API
#endif
