#include <phnt_windows.h>
#define PHNT_VERSION PHNT_20H1

#include <phnt.h>
#include <stdio.h>
#include <conio.h>
#include "helper.h"
#include "resource.h"

NTSTATUS CreateSimplePowerRequest(
    _Out_ PHANDLE PowerRequestHandle,
    _In_opt_ PCWSTR ReasonMessage
)
{
    COUNTED_REASON_CONTEXT context = { 0 };

    context.Version = POWER_REQUEST_CONTEXT_VERSION;
    context.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;

    if (ReasonMessage)
        RtlInitUnicodeString(&context.SimpleString, ReasonMessage);
    else
        context.Flags |= POWER_REQUEST_CONTEXT_NOT_SPECIFIED;

    return NtPowerInformation(
        PowerRequestCreate,
        &context,
        sizeof(COUNTED_REASON_CONTEXT),
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
    COUNTED_REASON_CONTEXT context = { 0 };

    context.Version = POWER_REQUEST_CONTEXT_VERSION;
    context.Flags = POWER_REQUEST_CONTEXT_DETAILED_STRING;

    RtlInitUnicodeString(&context.ResourceFileName, ModuleFileName);
    context.ResourceReasonId = MessageId;

    if (ParameterCount && MessageParameters)
    {
        // Allocate memory for supplying parameters
        context.ReasonStrings = RtlAllocateHeap(
            RtlGetCurrentPeb()->ProcessHeap,
            0,
            sizeof(UNICODE_STRING) * ParameterCount
        );

        if (!context.ReasonStrings)
            return STATUS_NO_MEMORY;

        context.StringCount = ParameterCount;

        // Set references to each parameter
        for (ULONG i = 0; i < ParameterCount; i++)
            RtlInitUnicodeString(&context.ReasonStrings[i], MessageParameters[i]);
    }
    
    NTSTATUS status = NtPowerInformation(
        PowerRequestCreate,
        &context,
        sizeof(COUNTED_REASON_CONTEXT),
        PowerRequestHandle,
        sizeof(HANDLE)
    );

    // Clean-up
    RtlFreeHeap(RtlGetCurrentPeb()->ProcessHeap, 0, context.ReasonStrings);

    return status;
}

NTSTATUS IssueActionPowerRequest(
    _In_ HANDLE PowerRequestHandle,
    _In_ POWER_REQUEST_TYPE RequestType,
    _In_ BOOLEAN Enable
)
{
    POWER_REQUEST_ACTION info = { 0 };
    ULONG size = sizeof(POWER_REQUEST_ACTION);

    info.PowerRequest = PowerRequestHandle;
    info.RequestType = RequestType;
    info.Enable = Enable;

    // Windows 7 does not know about the last field, exclude it
    if (RtlGetCurrentPeb()->OSMajorVersion == 6 && RtlGetCurrentPeb()->OSMinorVersion == 1)
        size = FIELD_OFFSET(POWER_REQUEST_ACTION, TargetProcess);

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
