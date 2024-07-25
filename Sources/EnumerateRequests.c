#include <phnt_windows.h>
#define PHNT_VERSION PHNT_20H1

#include <phnt.h>
#include <subprocesstag.h>
#include <stdio.h>
#include "helper.h"

ULONG DisplayRequest(
    _In_ PPOWER_REQUEST Request,
    _In_ POWER_REQUEST_TYPE RequestType
)
{
    PDIAGNOSTIC_BUFFER diagnosticBuffer;

    // Determine if the request matches the type
    if ((ULONG)RequestType >= SupportedModeCount || !Request->V4.PowerRequestCount[RequestType])
        return FALSE;

    // The location of the request's body depends on the supported modes
    switch (SupportedModeCount)
    {
        case POWER_REQUEST_SUPPORTED_TYPES_V1:
            diagnosticBuffer = &Request->V1.DiagnosticBuffer;
            break;

        case POWER_REQUEST_SUPPORTED_TYPES_V2:
            diagnosticBuffer = &Request->V2.DiagnosticBuffer;
            break;

        case POWER_REQUEST_SUPPORTED_TYPES_V3:
            diagnosticBuffer = &Request->V3.DiagnosticBuffer;
            break;

        case POWER_REQUEST_SUPPORTED_TYPES_V4:
            diagnosticBuffer = &Request->V4.DiagnosticBuffer;
            break;

        default:
            return FALSE;
    }

    PCWSTR requesterName = L"Unknown";
    PCWSTR requesterDetails = NULL;    
    TAG_INFO_NAME_FROM_TAG serviceInfo = { 0 };

    // Diagnostic info depends on the requester type
    switch (diagnosticBuffer->CallerType)
    {
        case KernelRequester:
            wprintf_s(L"[DRIVER] ");

            // For drivers, collecn the path and description
            if (diagnosticBuffer->DeviceDescriptionOffset)
                requesterName = (PCWSTR)RtlOffsetToPointer(diagnosticBuffer, diagnosticBuffer->DeviceDescriptionOffset);
            else
                requesterName = L"Legacy Kernel Caller";

            if (diagnosticBuffer->DevicePathOffset)
                requesterDetails = (PCWSTR)RtlOffsetToPointer(diagnosticBuffer, diagnosticBuffer->DevicePathOffset);

            break;

        case UserProcessRequester:
        case UserSharedServiceRequester:
            wprintf_s(diagnosticBuffer->CallerType == UserProcessRequester ?
                L"[PROCESS (PID %d)] " :
                L"[SERVICE (PID %d)] ",
                diagnosticBuffer->ProcessId);

            // Collect the process name for processes and services
            if (diagnosticBuffer->ProcessImageNameOffset)
                requesterName = (PCWSTR)RtlOffsetToPointer(diagnosticBuffer, diagnosticBuffer->ProcessImageNameOffset);

            // For services, convert their tags to names
            if (diagnosticBuffer->CallerType == UserSharedServiceRequester)
            {
                PQUERY_TAG_INFORMATION I_QueryTagInformation = I_QueryTagInformationLoader();

                serviceInfo.InParams.dwPid = diagnosticBuffer->ProcessId;
                serviceInfo.InParams.dwTag = diagnosticBuffer->ServiceTag;

                if (I_QueryTagInformation &&
                    I_QueryTagInformation(NULL, eTagInfoLevelNameFromTag, &serviceInfo) == ERROR_SUCCESS)
                    requesterDetails = serviceInfo.OutParams.pszName;
            }

            break;

        default:
            wprintf_s(L"[UNKNOWN] ");
            break;
    }

    // Power requests maintain a counter
    if (Request->V4.PowerRequestCount[RequestType] > 1)
        wprintf_s(L"[%d times] ", Request->V4.PowerRequestCount[RequestType]);

    if (requesterDetails)
        wprintf_s(L"%s (%s)\r\n", requesterName, requesterDetails);
    else
        wprintf_s(L"%s\r\n", requesterName);

    // The diagnostic buffer also stores the reason
    if (diagnosticBuffer->ReasonOffset)
    {
        PCOUNTED_REASON_CONTEXT_RELATIVE reason =
            (PCOUNTED_REASON_CONTEXT_RELATIVE)RtlOffsetToPointer(diagnosticBuffer, diagnosticBuffer->ReasonOffset);
        
        if (reason->Flags & POWER_REQUEST_CONTEXT_SIMPLE_STRING)
        {
            // Simple strings are packed into the buffer

            wprintf_s(L"%s\r\n", (PCWCHAR)RtlOffsetToPointer(reason, reason->SimpleStringOffset));
        }
        else if (reason->Flags & POWER_REQUEST_CONTEXT_DETAILED_STRING)
        {
            // Detailed strings are located in an external module

            HMODULE hModule = LoadLibraryExW(
                (PCWSTR)RtlOffsetToPointer(reason, reason->ResourceFileNameOffset),
                NULL,
                LOAD_LIBRARY_AS_DATAFILE
            );

            if (hModule)
            {
                PCWSTR reasonString;
                int reasonLength = LoadStringW(
                    hModule,
                    reason->ResourceReasonId,
                    (LPWSTR)&reasonString,
                    0
                );

                // TODO: substitute caller-supplied parameters

                if (reasonLength)
                    wprintf_s(L"%s\r\n", reasonString);

                // Clean-up
                FreeLibrary(hModule);
            }
        }
    }

    // Clean-up
    if (serviceInfo.OutParams.pszName)
        LocalFree(serviceInfo.OutParams.pszName);

    return TRUE;
}

void DisplayRequests(
    _In_ PPOWER_REQUEST_LIST RequestList,
    _In_ POWER_REQUEST_TYPE Condition,
    _In_ LPCWSTR Caption
)
{
    wprintf_s(L"%s:\r\n", Caption);

    ULONG found = FALSE;

    for (ULONG i = 0; i < RequestList->Count; i++)
    {
        found = DisplayRequest(
            (PPOWER_REQUEST)RtlOffsetToPointer(RequestList, RequestList->PowerRequestOffsets[i]),
            Condition
        ) || found;
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

    NTSTATUS status;
    PPOWER_REQUEST_LIST buffer;
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
        }

    } while (status == STATUS_BUFFER_TOO_SMALL);

    if (!IsSuccess(status, L"Querying power request list") || !buffer)
        return 1;

    InitializeSupportedModeCount();

    DisplayRequests(buffer, PowerRequestDisplayRequiredInternal, L"DISPLAY");
    DisplayRequests(buffer, PowerRequestSystemRequiredInternal, L"SYSTEM");
    DisplayRequests(buffer, PowerRequestAwayModeRequiredInternal, L"AWAYMODE");
    DisplayRequests(buffer, PowerRequestExecutionRequiredInternal, L"EXECUTION");
    DisplayRequests(buffer, PowerRequestPerfBoostRequiredInternal, L"PERFBOOST");
    DisplayRequests(buffer, PowerRequestActiveLockScreenInternal, L"ACTIVELOCKSCREEN");

    RtlFreeHeap(RtlGetCurrentPeb()->ProcessHeap, 0, buffer);

    return 0;
}
