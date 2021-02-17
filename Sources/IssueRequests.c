#include <phnt_windows.h>
#define PHNT_VERSION PHNT_20H1

#include <phnt.h>
#include <stdio.h>
#include "helper.h"
#include "power.h"

int main()
{
    // Do not allow running under WoW64
    if (IsWoW64())
        return 1;

    return 0;
}
