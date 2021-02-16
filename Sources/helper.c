#include "helper.h"
#include <stdio.h>
#include "power_requests.h"

ULONG IsSuccess(_In_ NTSTATUS Status, _In_ LPCWSTR Where)
{
    if (!NT_SUCCESS(Status))
    {
        wprintf_s(L"%s failed with 0x%0.8x", Where, Status);
        return FALSE;
    }

    return TRUE;
}

ULONG IsWoW64(void)
{
#ifdef _WIN64
    return FALSE;
#else
    ULONG_PTR isWoW64 = FALSE;

    NTSTATUS status = NtQueryInformationProcess(
        NtCurrentProcess(),
        ProcessWow64Information,
        &isWoW64,
        sizeof(isWoW64),
        NULL
    );

    if (!IsSuccess(status, L"WoW64 check"))
        return TRUE; // Assume we are under WoW64

    if (isWoW64)
        wprintf_s(L"Cannot run under WoW64, use the 64-bit version instead.");

    return !!isWoW64;
#endif
}

ULONG SupportedModeCount = 0;

void InitializeSupportedModeCount(void)
{
    POWER_REQUEST request;
    RTL_OSVERSIONINFOEXW versionInfo = { sizeof(RTL_OSVERSIONINFOEXW) };

    if (!NT_SUCCESS(RtlGetVersion(&versionInfo)))
        return;

    if (versionInfo.dwMajorVersion > 10 ||
        (versionInfo.dwMajorVersion == 10 && (versionInfo.dwMinorVersion > 0 ||
            (versionInfo.dwMinorVersion == 0 && versionInfo.dwBuildNumber >= 14393))))
    {
        // Windows 10 RS1+
        SupportedModeCount = POWER_REQUEST_SUPPORTED_MODES_V3;
    }
    else if (versionInfo.dwMajorVersion > 6 ||
        (versionInfo.dwMajorVersion == 6 && versionInfo.dwMinorVersion >= 3))
    {
        // Windows 8.1+
        SupportedModeCount = POWER_REQUEST_SUPPORTED_MODES_V2;
    }
    else
    {
        // Windows 7+
        SupportedModeCount = POWER_REQUEST_SUPPORTED_MODES_V1;
    }
}

PQUERY_TAG_INFORMATION I_QueryTagInformationLoader()
{
    static PQUERY_TAG_INFORMATION I_QueryTagInformationCache = NULL;

    // Use the cached address
    if (I_QueryTagInformationCache)
        return I_QueryTagInformationCache;

    UNICODE_STRING dllName;
    PVOID hAdvApi32;

    RtlInitUnicodeString(&dllName, L"advapi32.dll");

    if (NT_SUCCESS(LdrLoadDll(NULL, NULL, &dllName, &hAdvApi32)))
    {
        ANSI_STRING functionName;

        RtlInitAnsiString(&functionName, "I_QueryTagInformation");

        // Locate the function
        NTSTATUS status = LdrGetProcedureAddress(
            hAdvApi32,
            &functionName,
            0,
            (PVOID *)&I_QueryTagInformationCache
        );

        if (!NT_SUCCESS(status))
            I_QueryTagInformationCache = NULL;

        // Do not unload the DLL
    }

    return I_QueryTagInformationCache;
}
