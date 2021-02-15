#include <phnt_windows.h>
#include <phnt.h>

typedef struct _POWER_REQUEST_LIST {
    ULONG_PTR cElements;
    ULONG_PTR OffsetsToRequests[ANYSIZE_ARRAY];
} POWER_REQUEST_LIST, *PPOWER_REQUEST_LIST;

typedef enum _POWER_REQUEST_ORIGIN {
    POWER_REQUEST_ORIGIN_DRIVER = 0,
    POWER_REQUEST_ORIGIN_PROCESS = 1,
    POWER_REQUEST_ORIGIN_SERVICE = 2
} POWER_REQUEST_ORIGIN;

typedef struct _POWER_REQUEST_BODY {
    ULONG_PTR cbSize;
    POWER_REQUEST_ORIGIN Origin;
    ULONG_PTR OffsetToRequester;
    ULONG ProcessId;
    ULONG Unknown;
    ULONG_PTR OffsetToContext;
} POWER_REQUEST_BODY, *PPOWER_REQUEST_BODY;

typedef struct _POWER_REQUEST {
    union
    {
#if (PHNT_VERSION >= PHNT_WIN7)
        struct
        {
            ULONG Reserved;
            ULONG RequiredDisplay;
            ULONG RequiredSystem;
            ULONG RequiredAwayMode;
            POWER_REQUEST_BODY Body;
        } V1;
#endif
#if (PHNT_VERSION >= PHNT_WINBLUE) // Or maybe Windows 8
        struct
        {
            ULONG Reserved;
            ULONG RequiredDisplay;
            ULONG RequiredSystem;
            ULONG RequiredAwayMode;
            ULONG RequiredExecution;
            ULONG RequiredPerfBoost;
            POWER_REQUEST_BODY Body;
        } V2;
#endif
#if (PHNT_VERSION >= PHNT_REDSTONE)
        struct
        {
            ULONG Reserved;
            ULONG RequiredDisplay;
            ULONG RequiredSystem;
            ULONG RequiredAwayMode;
            ULONG RequiredExecution;
            ULONG RequiredPerfBoost;
            ULONG RequiredActiveLockScreen;
            POWER_REQUEST_BODY Body;
        } V3;
#endif
    };
} POWER_REQUEST, *PPOWER_REQUEST;
