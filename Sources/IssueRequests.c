#include <phnt_windows.h>
#define PHNT_VERSION PHNT_20H1

#include <phnt.h>
#include <stdio.h>
#include <conio.h>
#include "helper.h"
#include "power.h"

NTSTATUS CreateSimplePowerRequest(
    _Out_ PHANDLE PowerRequestHandle,
    _In_opt_ PCWSTR ReasonMessage
)
{
    POWER_REQUEST_CONTEXT_IN requestContext = { 0 };

    requestContext.Version = POWER_REQUEST_CONTEXT_VERSION;
    requestContext.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;

    if (ReasonMessage)
        RtlInitUnicodeString(&requestContext.SimpleReasonString, ReasonMessage);
    else
        requestContext.Flags |= POWER_REQUEST_CONTEXT_NOT_SPECIFIED;

    return NtPowerInformation(
        PowerRequestCreate,
        &requestContext,
        sizeof(POWER_REQUEST_CONTEXT_IN),
        PowerRequestHandle,
        sizeof(HANDLE)
    );
}

NTSTATUS IssueActionPowerRequest(
    _In_ HANDLE PowerRequestHandle,
    _In_ POWER_REQUEST_TYPE RequestType,
    _In_ BOOLEAN Enable
)
{
    POWER_REQUEST_SET_INFORMATION info = { 0 };

    info.PowerRequestHandle = PowerRequestHandle;
    info.RequestType = RequestType;
    info.Enable = Enable;

    return NtPowerInformation(
        PowerRequestAction,
        &info,
        sizeof(POWER_REQUEST_SET_INFORMATION),
        NULL,
        0
    );
}

int main()
{
    // Do not allow running under WoW64
    if (IsWoW64())
        return 1;

    HANDLE hPowerRequest;

    // Test simple power requests
    NTSTATUS status = CreateSimplePowerRequest(
        &hPowerRequest,
        L"Simple reason string"
    );

    if (!IsSuccess(status, L"Power request creation"))
        return 1;

    // Ask for the display to be on
    status = IssueActionPowerRequest(
        hPowerRequest,
        PowerRequestDisplayRequired,
        TRUE
    );

    if (!IsSuccess(status, L"Enabling power request"))
        return 1;

    wprintf_s(L"Success; press any key to exit...");

    // Also try more exotic actions that are blocked by the documented API
    IssueActionPowerRequest(
        hPowerRequest,
        PowerRequestPerfBoostRequired,
        TRUE
    );

    _getch();
    return 0;
}
