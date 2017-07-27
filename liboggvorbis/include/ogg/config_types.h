
#ifndef _CONFIG_TYPES_H
#define _CONFIG_TYPES_H

#if defined(__ANDROID__) || defined(__IOS__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(GEKKO)

/* Android or iOS compiler */
#	include <stdint.h>
typedef int16_t ogg_int16_t;
typedef uint16_t ogg_uint16_t;
typedef int32_t ogg_int32_t;
typedef uint32_t ogg_uint32_t;
typedef int64_t ogg_int64_t;

#else

#	error You should add the definitions of ogg_*_t types for your platform here.

#endif

#endif

