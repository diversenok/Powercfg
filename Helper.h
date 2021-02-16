#include <phnt_windows.h>
#define PHNT_VERSION PHNT_20H1
#include <phnt.h>

ULONG IsSuccess(
    _In_ NTSTATUS Status,
    _In_ LPCWSTR Where
);

ULONG IsWoW64(void);

ULONG SupportedModeCount;

void InitializeSupportedModeCount(void);
