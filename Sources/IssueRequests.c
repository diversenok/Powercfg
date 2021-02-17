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

int main()
{
    // Do not allow running under WoW64
    if (IsWoW64())
        return 1;

    HANDLE hPowerRequest;

    // Test simple power requests
    NTSTATUS status = CreateSimplePowerRequest(
        &hPowerRequest,
        L"Testing Power Requests"
    );

    if (NT_SUCCESS(status))
        wprintf_s(L"Success, handle value is %p\r\n", hPowerRequest);
    else
        wprintf_s(L"NtPowerInformation failed with 0x%0.8x", status);

    _getch();
    return 0;
}
