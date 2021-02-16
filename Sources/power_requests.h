#include <phnt_windows.h>
#include <phnt.h>

typedef struct _POWER_REQUEST_LIST
{
    ULONG_PTR cElements;
    ULONG_PTR OffsetsToRequests[ANYSIZE_ARRAY];
} POWER_REQUEST_LIST, *PPOWER_REQUEST_LIST;

typedef enum _POWER_REQUEST_ORIGIN
{
    POWER_REQUEST_ORIGIN_DRIVER = 0,
    POWER_REQUEST_ORIGIN_PROCESS = 1,
    POWER_REQUEST_ORIGIN_SERVICE = 2
} POWER_REQUEST_ORIGIN;

typedef struct _POWER_REQUEST_BODY
{
    ULONG_PTR cbSize;
    POWER_REQUEST_ORIGIN Origin;
    ULONG_PTR OffsetToRequester;
    union
    {
        struct
        {
            ULONG ProcessId;
            ULONG ServiceTag;
        };
        ULONG_PTR OffsetToDriverName;
    };    
    ULONG_PTR OffsetToContext;
} POWER_REQUEST_BODY, *PPOWER_REQUEST_BODY;

// POWER_REQUEST_TYPE
// Note: We don't use an enum since it conflicts with the Windows SDK.
#define PowerRequestDisplayRequired 0
#define PowerRequestSystemRequired 1
#define PowerRequestAwayModeRequired 2
#define PowerRequestExecutionRequired 3        // Windows 8.1+ (maybe Windows 8?)
#define PowerRequestPerfBoostRequired 4        // Windows 8.1+ (maybe Windows 8?)
#define PowerRequestActiveLockScreenRequired 5 // Windows 10 RS1+

#define POWER_REQUEST_SUPPORTED_MODES_V1 3
#define POWER_REQUEST_SUPPORTED_MODES_V2 5
#define POWER_REQUEST_SUPPORTED_MODES_V3 6

typedef struct _POWER_REQUEST
{
    union
    {
        struct
        {
            ULONG Reserved;
            ULONG Requires[POWER_REQUEST_SUPPORTED_MODES_V1];
            POWER_REQUEST_BODY Body;
        } V1;
#if (PHNT_VERSION >= PHNT_WINBLUE)
        struct
        {
            ULONG Reserved;
            ULONG Requires[POWER_REQUEST_SUPPORTED_MODES_V2];
            POWER_REQUEST_BODY Body;
        } V2;
#endif
#if (PHNT_VERSION >= PHNT_REDSTONE)
        struct
        {
            ULONG Reserved;
            ULONG Requires[POWER_REQUEST_SUPPORTED_MODES_V3];
            POWER_REQUEST_BODY Body;
        } V3;
#endif
    };
} POWER_REQUEST, *PPOWER_REQUEST;

typedef struct _POWER_REQUEST_CONTEXT_INFO
{
    ULONG Flags; // POWER_REQUEST_CONTEXT_SIMPLE_STRING or POWER_REQUEST_CONTEXT_DETAILED_STRING
    union
    {
        struct
        {
            ULONG_PTR OffsetToModuleName;
            USHORT LocalizedReasonId;
            ULONG ReasonStringCount;
            ULONG_PTR OffsetToReasonStrings;
        } Detailed;
        ULONG_PTR OffsetToSimpleString;
    };
} POWER_REQUEST_CONTEXT_INFO, *PPOWER_REQUEST_CONTEXT_INFO;
