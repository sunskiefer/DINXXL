#pragma once

#ifndef MINIZ_EXPORT
  #if defined(_WIN32) && defined(MINIZ_SHARED)
    #ifdef miniz_EXPORTS
      #define MINIZ_EXPORT __declspec(dllexport)
    #else
      #define MINIZ_EXPORT __declspec(dllimport)
    #endif
  #else
    #define MINIZ_EXPORT
  #endif
#endif

#ifndef MINIZ_NO_EXPORT
  #define MINIZ_NO_EXPORT
#endif
