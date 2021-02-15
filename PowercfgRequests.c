#include <phnt_windows.h>
#define PHNT_VERSION PHNT_20H1

#include <phnt.h>
#include <stdio.h>
#include "Helper.h"
#include "powerrequests.h"

ULONG ShouldDisplay(
    _In_ PPOWER_REQUEST Request,
    _In_ POWER_REQUEST_TYPE Condition
)
{
    switch (Condition)
    {
        case PowerRequestDisplayRequired:
            return Request->V1.RequiredDisplay;

        case PowerRequestSystemRequired:
            return Request->V1.RequiredSystem;

        case PowerRequestAwayModeRequired:
            return Request->V1.RequiredAwayMode;

        case PowerRequestExecutionRequired:
            return RequestVersion >= REQUEST_VERSION_2 && Request->V2.RequiredExecution;

        case PowerRequestPerfBoostRequired:
            return RequestVersion >= REQUEST_VERSION_2 && Request->V2.RequiredPerfBoost;

        case PowerRequestActiveLockScreenRequired:
            return RequestVersion >= REQUEST_VERSION_3 && Request->V3.RequiredActiveLockScreen;

        default:
            return FALSE;
    }
}

ULONG DisplayRequest(
    _In_ PPOWER_REQUEST Request,
    _In_ POWER_REQUEST_TYPE Condition
)
{
    if (!ShouldDisplay(Request, Condition))
        return FALSE;

    PPOWER_REQUEST_BODY requestBody;

    // Determine the structure based on the current version of Windows
    switch (RequestVersion)
    {
        case REQUEST_VERSION_1:
            requestBody = &Request->V1.Body;
            break;

        case REQUEST_VERSION_2:
            requestBody = &Request->V2.Body;
            break;

        case REQUEST_VERSION_3:
            requestBody = &Request->V3.Body;
            break;

        default:
            return FALSE;
    }

    // Print the requester type
    switch (requestBody->Origin)
    {
        case POWER_REQUEST_ORIGIN_DRIVER:
            wprintf_s(L"[DRIVER] ");
            break;
        case POWER_REQUEST_ORIGIN_PROCESS:
            wprintf_s(L"[PROCESS (PID %d)] ", requestBody->ProcessId);
            break;
        case POWER_REQUEST_ORIGIN_SERVICE:
            wprintf_s(L"[SERVICE (PID %d)] ", requestBody->ProcessId);
            break;
        default:
            wprintf_s(L"[UNKNOWN] ");
            break;
    }

    wprintf_s(L"\r\n");
}

void DisplayRequests(
    _In_ PPOWER_REQUEST_LIST buffer,
    _In_ ULONG bufferLength,
    _In_ POWER_REQUEST_TYPE Condition,
    _In_ LPCWSTR Caption
)
{
    wprintf_s(Caption);

    ULONG Count = 0;

    for (ULONG i = 0; i < buffer->cElements; i++)
    {
        if (buffer->OffsetsToRequests[i] + MinimalRequestSize > (ULONG_PTR)bufferLength)
        {
            wprintf_s(L"Parsing error: offset to request is too big\r\n");
            return;
        }

        PPOWER_REQUEST request = (PPOWER_REQUEST)((UINT_PTR)buffer + buffer->OffsetsToRequests[i]);

        if (DisplayRequest(request, Condition))
            Count++;
    }

    if (Count == 0)
        wprintf_s(L"None.\r\n");

    wprintf_s(L"\r\n");
}

int main()
{
    // Do not allow running under WoW64
    if (IsWoW64())
        return 1;

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PPOWER_REQUEST_LIST buffer = NULL;
    ULONG bufferLength = 4096;

    do
    {
        buffer = RtlAllocateHeap(
            RtlGetCurrentPeb()->ProcessHeap,
            0,
            bufferLength
        );

        if (!buffer)
        {
            wprintf_s(L"Not enough memory");
            return 1;
        }

        // Query the power request list
        status = NtPowerInformation(
            GetPowerRequestList,
            NULL,
            0,
            buffer,
            bufferLength
        );

        if (!NT_SUCCESS(status))
        {
            RtlFreeHeap(RtlGetCurrentPeb()->ProcessHeap, 0, buffer);
            buffer = NULL;

            // Prepare for expansion
            bufferLength += 4096;

            if (bufferLength >= 256 * 1024 * 1024)
            {
                wprintf_s(L"Required buffer is too big.");
                return 1;
            }
        }

    } while (status == STATUS_BUFFER_TOO_SMALL);

    if (!IsSuccess(status, L"Querying power request list") || !buffer)
        return 1;

    if (buffer->cElements * sizeof(UINT_PTR) >= bufferLength)
    {
        wprintf_s(L"Parsing error: too many elements");
        goto CLEANUP;
    }

    InitializeRequestVersion();

    DisplayRequests(buffer, bufferLength, PowerRequestDisplayRequired, L"DISPLAY:\r\n");
    DisplayRequests(buffer, bufferLength, PowerRequestSystemRequired, L"SYSTEM:\r\n");
    DisplayRequests(buffer, bufferLength, PowerRequestAwayModeRequired, L"AWAYMODE:\r\n");
    DisplayRequests(buffer, bufferLength, PowerRequestExecutionRequired, L"EXECUTION:\r\n");
    DisplayRequests(buffer, bufferLength, PowerRequestPerfBoostRequired, L"PERFBOOST:\r\n");
    DisplayRequests(buffer, bufferLength, PowerRequestActiveLockScreenRequired, L"ACTIVELOCKSCREEN:\r\n");

CLEANUP:
    RtlFreeHeap(RtlGetCurrentPeb()->ProcessHeap, 0, buffer);

    return 0;
}
