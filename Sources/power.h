#include <phnt_windows.h>
#include <phnt.h>

#define  POWER_REQUEST_CONTEXT_NOT_SPECIFIED DIAGNOSTIC_REASON_NOT_SPECIFIED

typedef struct _POWER_REQUEST_CONTEXT_IN
{
    ULONG Version;
    ULONG Flags;
    union
    {
        struct
        {
            UNICODE_STRING LocalizedReasonModule;
            USHORT LocalizedReasonId;
            ULONG ReasonStringCount;
            PUNICODE_STRING ReasonStrings[ANYSIZE_ARRAY];
        } Detailed;
        UNICODE_STRING SimpleReasonString;
    };
} POWER_REQUEST_CONTEXT_IN, *PPOWER_REQUEST_CONTEXT_IN;

// POWER_REQUEST_TYPE
// Note: We don't use an enum since it conflicts with the Windows SDK.
#define PowerRequestDisplayRequired 0
#define PowerRequestSystemRequired 1
#define PowerRequestAwayModeRequired 2
#define PowerRequestExecutionRequired 3        // Windows 8+
#define PowerRequestPerfBoostRequired 4        // Windows 8+
#define PowerRequestActiveLockScreenRequired 5 // Windows 10 RS1+ (reserved on Windows 8)
// Values 6 and 7 are reserved for Windows 8 only
#define PowerRequestFullScreenVideoRequired 8  // Windows 8 only

typedef struct _POWER_REQUEST_SET_INFORMATION
{
    HANDLE PowerRequestHandle;
    POWER_REQUEST_TYPE RequestType;
    BOOLEAN Enable;
    HANDLE Reserved;
} POWER_REQUEST_SET_INFORMATION, *PPOWER_REQUEST_SET_INFORMATION;

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

// The number of supported request modes per version
#define POWER_REQUEST_SUPPORTED_MODES_V1 3 // Windows 7
#define POWER_REQUEST_SUPPORTED_MODES_V2 9 // Windows 8
#define POWER_REQUEST_SUPPORTED_MODES_V3 5 // Windows 8.1 and Windows 10 TH1-TH2
#define POWER_REQUEST_SUPPORTED_MODES_V4 6 // Windows 10 RS1+

typedef struct _POWER_REQUEST
{
    union
    {
#if (PHNT_VERSION >= PHNT_WIN7)
        struct
        {
            ULONG Reserved;
            ULONG Requires[POWER_REQUEST_SUPPORTED_MODES_V1];
            POWER_REQUEST_BODY Body;
        } V1;
#endif
#if (PHNT_VERSION >= PHNT_WIN8)
        struct
        {
            ULONG Reserved;
            ULONG Requires[POWER_REQUEST_SUPPORTED_MODES_V2];
            POWER_REQUEST_BODY Body;
        } V2;
#endif
#if (PHNT_VERSION >= PHNT_WINBLUE)
        struct
        {
            ULONG Reserved;
            ULONG Requires[POWER_REQUEST_SUPPORTED_MODES_V3];
            POWER_REQUEST_BODY Body;
        } V3;
#endif
#if (PHNT_VERSION >= PHNT_REDSTONE)
        struct
        {
            ULONG Reserved;
            ULONG Requires[POWER_REQUEST_SUPPORTED_MODES_V4];
            POWER_REQUEST_BODY Body;
        } V4;
#endif
    };
} POWER_REQUEST, *PPOWER_REQUEST;

typedef struct _POWER_REQUEST_CONTEXT_OUT
{
    ULONG Flags;
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
} POWER_REQUEST_CONTEXT_OUT, *PPOWER_REQUEST_CONTEXT_OUT;
