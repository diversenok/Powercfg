#include <phnt_windows.h>
#define PHNT_VERSION PHNT_20H1
#include <phnt.h>

ULONG IsSuccess(
    _In_ NTSTATUS Status,
    _In_ LPCWSTR Where
);

ULONG IsWoW64(void);

typedef enum _REQUEST_VERSION {
    REQUEST_VERSION_UNKNOWN,
    REQUEST_VERSION_1,
    REQUEST_VERSION_2,
    REQUEST_VERSION_3
} REQUEST_VERSION;

REQUEST_VERSION RequestVersion;
ULONG MinimalRequestSize;

void InitializeRequestVersion(void);

// Extending POWER_REQUEST_TYPE
#define PowerRequestPerfBoostRequired 4
#define PowerRequestActiveLockScreenRequired 5
