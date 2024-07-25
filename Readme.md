# Reversing Powercfg

This is a demo project on how to issue and enumerate power requests on Windows using Native API. Programs (such as media players) can create power requests by using the [`PowerCreateRequest`](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powercreaterequest) function, followed by calls to [`PowerSetRequest`](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powersetrequest). It allows them to request the system to ignore its default settings of dimming the display or shutting down after a timeout of inactivity. The built-in **powercfg** tool allows viewing such requests when called with a `/requests` parameter. Under the hood, every piece of this functionality relies on calling [`NtPowerInformation`](https://ntdoc.m417z.com/ntpowerinformation) with several undocumented info classes.

The purpose of this project is to reverse and document the underlying structures for the following info classes:
 - `PowerRequestCreate`
 - `PlmPowerRequestCreate`
 - `PowerRequestAction`
 - `GetPowerRequestList`

## Request Types

Each power request can include any combination of the following operations:

 - **DISPLAY** - does not allow dimming the display after a timeout.
 - **SYSTEM** - keeps the system awake, preventing automatic shutdown.
 - **AWAYMODE** - keeps the system awake for background processing while it appears to be sleeping. When explicitly requesting sleep, the computer merely turns off the display and sound.
 - **EXECUTION** - overrides Process Lifetime Management mechanisms; requires Windows 8 and higher.
 - **PERFBOOST** - unknown; requires Windows 8 and higher.
 - **ACTIVELOCKSCREEN** - unknown; only allowed for processes in an interactive session; requires Windows 10 RS1 and higher.

The first four types are defined in Windows SDK's `winnt.h`:

```c
typedef enum _POWER_REQUEST_TYPE {
    PowerRequestDisplayRequired,
    PowerRequestSystemRequired,
    PowerRequestAwayModeRequired,
    PowerRequestExecutionRequired
} POWER_REQUEST_TYPE, *PPOWER_REQUEST_TYPE;
```

The official names for the rest are known from an extended enumeration recovered from debug symbols:

```c
typedef enum _POWER_REQUEST_TYPE_INTERNAL
{
    PowerRequestDisplayRequiredInternal = 0,
    PowerRequestSystemRequiredInternal = 1,
    PowerRequestAwayModeRequiredInternal = 2,
    PowerRequestExecutionRequiredInternal = 3, // Windows 8+
    PowerRequestPerfBoostRequiredInternal = 4, // Windows 8+
    PowerRequestActiveLockScreenInternal = 5,  // Windows 10 RS1+ (reserved on Windows 8)
    PowerRequestFullScreenVideoRequired = 9    // Windows 8 only
} POWER_REQUEST_TYPE_INTERNAL;
```

## Info classes

#### PowerRequestCreate

This information class captures the provided diagnostic reason and creates a PowerRequest kernel object, returning an exclusive handle to it. The reason parameter can either be a simple message or a localized string resource in a DLL (for which you can also supply parameters). The request capture information about its creator is not active by default. A documented function [with a similar name](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powercreaterequest) exposes the full functionality of this info class.

#### PlmPowerRequestCreate

This information class has the same parameters and works similar to the previous one but creates a Process Lifetime Management request. PLM requests only support the EXECUTION mode, cannot be used by packaged applications (i.e., by processes with a `WIN://SYSAPPID` token attribute), and require the caller to explicitly specify the target process handle upon performing an action. The process handle requires `PROCESS_SET_LIMITED_INFORMATION` access. PLM requests allow temporarily unfreezing a given UWP process.

#### PowerRequestAction

 This information class allows activating and deactivating required modes for a power request, one at a time. Although two documented functions ([`PowerSetRequest`](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powersetrequest) and [`PowerClearRequest`](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powerclearrequest)) partially expose its functionality, they only support DISPLAY, SYSTEM, AWAYMODE, and EXECUTION modes, blocking PERFBOOST and ACTIVELOCKSCREEN. Additionally, they do not allow supplying the target process handle so cannot be used on PLM requests.

#### GetPowerRequestList

This information class is mainly used by powercfg and requires administrative rights. It enumerates all outstanding system-wide (non-PLM) power requests, including those that are not currently affecting the system. Each entry contains the following information:

 - Active counter for each supported mode
 - The type of the request creator, being a process, a shared service, or a kernel driver
 - Device name and description for driver requests
 - Image name (in the native format), PID, and the service tag for user-mode creators
 - Diagnostic information (simple or localized reason message provided by the creator)
