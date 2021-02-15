#include <phnt_windows.h>
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

REQUEST_VERSION RequestVersion = REQUEST_VERSION_UNKNOWN;
ULONG MinimalRequestSize = 0;

void InitializeRequestVersion(void);
