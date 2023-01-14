#define VERSION_STR "1.0.7"
#define VERSION_NUMBER 7

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
