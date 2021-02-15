#include <phnt_windows.h>
#include <phnt.h>

typedef struct _POWER_REQUEST_LIST {
    ULONG_PTR cElements;
    ULONG_PTR OffsetsToRequests[ANYSIZE_ARRAY];
} POWER_REQUEST_LIST, *PPOWER_REQUEST_LIST;
