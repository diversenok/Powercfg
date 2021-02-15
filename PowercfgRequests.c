#include <phnt_windows.h>
#define PHNT_VERSION PHNT_20H1

#include <phnt.h>
#include <stdio.h>
#include "Helper.h"
#include "powerrequests.h"

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

    for (ULONG i = 0; i < buffer->cElements; i++)
    {
        if (buffer->OffsetsToRequests[i] >= (ULONG_PTR)bufferLength ||
            buffer->OffsetsToRequests[i] + MinimalRequestSize > (ULONG_PTR)bufferLength)
        {
            wprintf_s(L"Parsing error: offset to request is too big");
            goto CLEANUP;
        }

        PPOWER_REQUEST request = (PPOWER_REQUEST)((UINT_PTR)buffer + buffer->OffsetsToRequests[i]);
    }

CLEANUP:
    RtlFreeHeap(RtlGetCurrentPeb()->ProcessHeap, 0, buffer);

    return 0;
}
