#include <phnt_windows.h>
#define PHNT_VERSION PHNT_20H1

#include <phnt.h>
#include <stdio.h>
#include <conio.h>
#include "helper.h"
#include "power.h"
#include "resource.h"

NTSTATUS CreateSimplePowerRequest(
    _Out_ PHANDLE PowerRequestHandle,
    _In_opt_ PCWSTR ReasonMessage
)
{
    POWER_REQUEST_CONTEXT_IN context = { 0 };

    context.Version = POWER_REQUEST_CONTEXT_VERSION;
    context.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;

    if (ReasonMessage)
        RtlInitUnicodeString(&context.SimpleReasonString, ReasonMessage);
    else
        context.Flags |= POWER_REQUEST_CONTEXT_NOT_SPECIFIED;

    return NtPowerInformation(
        PowerRequestCreate,
        &context,
        sizeof(POWER_REQUEST_CONTEXT_IN),
        PowerRequestHandle,
        sizeof(HANDLE)
    );
}

NTSTATUS CreateDetailedPowerRequest(
    _Out_ PHANDLE PowerRequestHandle,
    _In_ PWSTR ModuleFileName,
    _In_ USHORT MessageId,
    _In_opt_ PWSTR* MessageParameters,
    _In_ ULONG ParameterCount
)
{
    POWER_REQUEST_CONTEXT_IN context = { 0 };

    context.Version = POWER_REQUEST_CONTEXT_VERSION;
    context.Flags = POWER_REQUEST_CONTEXT_DETAILED_STRING;

    RtlInitUnicodeString(&context.Detailed.LocalizedReasonModule, ModuleFileName);
    context.Detailed.LocalizedReasonId = MessageId;

    if (ParameterCount && MessageParameters)
    {
        // Allocate memory for supplying parameters
        context.Detailed.ReasonStrings = RtlAllocateHeap(
            RtlGetCurrentPeb()->ProcessHeap,
            0,
            sizeof(UNICODE_STRING) * ParameterCount
        );

        if (!context.Detailed.ReasonStrings)
            return STATUS_NO_MEMORY;

        context.Detailed.ReasonStringCount = ParameterCount;

        // Set references to each parameter
        for (ULONG i = 0; i < ParameterCount; i++)
            RtlInitUnicodeString(&context.Detailed.ReasonStrings[i], MessageParameters[i]);
    }
    
    NTSTATUS status = NtPowerInformation(
        PowerRequestCreate,
        &context,
        sizeof(POWER_REQUEST_CONTEXT_IN),
        PowerRequestHandle,
        sizeof(HANDLE)
    );

    // Clean-up
    RtlFreeHeap(RtlGetCurrentPeb()->ProcessHeap, 0, context.Detailed.ReasonStrings);

    return status;
}

NTSTATUS IssueActionPowerRequest(
    _In_ HANDLE PowerRequestHandle,
    _In_ POWER_REQUEST_TYPE RequestType,
    _In_ BOOLEAN Enable
)
{
    POWER_REQUEST_ACTION info = { 0 };
    SIZE_T size = sizeof(POWER_REQUEST_ACTION);

    info.PowerRequestHandle = PowerRequestHandle;
    info.RequestType = RequestType;
    info.Enable = Enable;

    // Windows 7 does not know about the last field, exclude it
    if (RtlGetCurrentPeb()->OSMajorVersion == 6 && RtlGetCurrentPeb()->OSMinorVersion == 1)
        size = FIELD_OFFSET(POWER_REQUEST_ACTION, TargetProcessHandle);

    return NtPowerInformation(
        PowerRequestAction,
        &info,
        size,
        NULL,
        0
    );
}

int main()
{
    // Do not allow running under WoW64
    if (IsWoW64())
        return 1;

    HANDLE hPowerRequest = NULL;

    // Test simple power requests
    NTSTATUS status = CreateSimplePowerRequest(
        &hPowerRequest,
        L"Simple reason string"
    );

    if (!IsSuccess(status, L"Creating power request"))
        return 1;

    // Ask for the display to be on
    status = IssueActionPowerRequest(
        hPowerRequest,
        PowerRequestDisplayRequired,
        TRUE
    );

    if (!IsSuccess(status, L"Activating power request"))
        goto CLEANUP;

    // Also try more exotic actions that are blocked by the documented API
    IssueActionPowerRequest(
        hPowerRequest,
        PowerRequestPerfBoostRequired,
        TRUE
    );

    wprintf_s(L"Success; You should see a power request with a simple message.\r\n");
    wprintf_s(L"Press any key to continue...\r\n");
    _getch();

    NtClose(hPowerRequest);

    // Test localizable power requests using a locally embedded resource
    status = CreateDetailedPowerRequest(
        &hPowerRequest,
        RtlGetCurrentPeb()->ProcessParameters->ImagePathName.Buffer,
        ID_EXAMPLE_STRING,
        NULL,
        0
    );

    if (!IsSuccess(status, L"Creating power request"))
        return 1;

    // Ask for the system to stay awake
    status = IssueActionPowerRequest(
        hPowerRequest,
        PowerRequestSystemRequired,
        TRUE
    );

    if (!IsSuccess(status, L"Activating power request"))
        goto CLEANUP;

    wprintf_s(L"Success; You should see a power request with a detailed message.\r\n");
    wprintf_s(L"Press any key to exit...\r\n");
    _getch();

CLEANUP:
    if (hPowerRequest)
        NtClose(hPowerRequest);

    return 0;
}
