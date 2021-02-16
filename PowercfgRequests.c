#include <phnt_windows.h>
#define PHNT_VERSION PHNT_20H1

#include <phnt.h>
#include <stdio.h>
#include "Helper.h"
#include "powerrequests.h"

ULONG DisplayRequest(
    _In_ PPOWER_REQUEST Request,
    _In_ POWER_REQUEST_TYPE RequestType
)
{
    PPOWER_REQUEST_BODY requestBody;

    // Determine if the request matches the type
    if (RequestType < 0 || RequestType >= SupportedModeCount || !Request->V3.Requires[RequestType])
        return FALSE;

    // The location of the request's body depends on the supported modes
    switch (SupportedModeCount)
    {
        case POWER_REQUEST_SUPPORTED_MODES_V1:
            requestBody = &Request->V1.Body;
            break;

        case POWER_REQUEST_SUPPORTED_MODES_V2:
            requestBody = &Request->V2.Body;
            break;

        case POWER_REQUEST_SUPPORTED_MODES_V3:
            requestBody = &Request->V3.Body;
            break;

        default:
            return FALSE;
    }

    // Print the requester kind
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

    PCWSTR requesterName = L"Legacy Kernel Caller";
    PCWSTR requesterDetails = NULL;

    // Retrieve general requester information
    if (requestBody->OffsetToRequester && requestBody->OffsetToRequester < requestBody->cbSize)
        requesterName = (PCWSTR)((UINT_PTR)requestBody + requestBody->OffsetToRequester);

    // For drivers, locate the full name
    if (requestBody->Origin == POWER_REQUEST_ORIGIN_DRIVER && requestBody->OffsetToDriverName != 0)
        requesterDetails = (PCWSTR)((UINT_PTR)requestBody + requestBody->OffsetToDriverName);

    if (requesterDetails)
        wprintf_s(L"%s (%s)\r\n", requesterName, requesterDetails);
    else
        wprintf_s(L"%s\r\n", requesterName);

    return TRUE;
}

void DisplayRequests(
    _In_reads_bytes_(RequestListSize) PPOWER_REQUEST_LIST RequestList,
    _In_ ULONG RequestListSize,
    _In_ POWER_REQUEST_TYPE Condition,
    _In_ LPCWSTR Caption
)
{
    wprintf_s(Caption);

    ULONG found = FALSE;

    for (ULONG i = 0; i < RequestList->cElements; i++)
    {
        if (RequestList->OffsetsToRequests[i] > (ULONG_PTR)RequestListSize)
        {
            wprintf_s(L"Parsing error: offset to request is too big\r\n");
            return;
        }

        PPOWER_REQUEST request = (PPOWER_REQUEST)((UINT_PTR)RequestList + RequestList->OffsetsToRequests[i]);

        if (DisplayRequest(request, Condition))
            found = TRUE;
    }

    if (!found)
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

    InitializeSupportedModeCount();

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
