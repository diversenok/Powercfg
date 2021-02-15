#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>
#include "powerrequests.h"

ULONG IsSuccess(NTSTATUS Status, LPCWSTR Where)
{
    if (!NT_SUCCESS(Status))
    {
        wprintf_s(L"%s failed with 0x%0.8x", Where, Status);
        return FALSE;
    }

    return TRUE;
}

ULONG IsWoW64()
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

int main()
{
    // Do not allow running under WoW64
    if (IsWoW64())
        return 1;

    NTSTATUS status;
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

    wprintf_s(L"Buffer size is %u\r\n", bufferLength);
    wprintf_s(L"Found %u power requests", (ULONG)buffer->cElements);

    RtlFreeHeap(RtlGetCurrentPeb()->ProcessHeap, 0, buffer);

    return 0;
}
