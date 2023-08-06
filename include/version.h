#define VERSION_STR "1.0.8"
#define VERSION_NUMBER 8

#ifdef DEBUG
    #define BUILD_STR "DEBUG"
#else
    #define BUILD_STR "RELEASE"
#endif

#ifdef HELTEC
    #define PLATFORM_STR "HELTEC"
#else
    #define PLATFORM_STR "POE"
#endif
