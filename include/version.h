#define VERSION_STR "1.0.9"
#define VERSION_NUMBER 9

#ifdef DEBUG
    #define BUILD_STR "DEBUG"
#else
    #define BUILD_STR "RELEASE"
#endif

#ifdef HELTEC
    #define PLATFORM_STR "HELTEC"
#endif
#ifdef HELTECV3
    #define PLATFORM_STR "HELTECV3"
#else
    #define PLATFORM_STR "POE"
#endif
