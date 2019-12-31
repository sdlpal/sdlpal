#define PACKAGE_VERSION "1.3.1"

/* for libopus */
#define OPUS_BUILD
#define ENABLE_HARDENING

#ifdef _MSC_VER
#define USE_ALLOCA
#define alloca _alloca
#else
#define VAR_ARRAYS
#define FORTIFY_SOURCE 2
#endif

/* for libopusfile */
#define HAVE_LRINTF
