# Powercfg

This is a demo project on how to issue and enumerate power requests on Windows using Native API. Programs (such as media players) can create power requests by using the [PowerCreateRequest](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powercreaterequest) function, followed by calls to [PowerSetRequest](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powersetrequest). It allows them to request the system to ignore its default settings of dimming the display or shutting down after a timeout of inactivity. The built-in **powercfg** tool allows viewing such requests when called with a `/requests` parameter. Under the hood, every piece of this functionality relies on calling [NtPowerInformation](https://github.com/processhacker/processhacker/blob/afda2a2dbf2d2e37abc0e3986c295ec279192eb4/phnt/include/ntpoapi.h#L244-L253) with several undocumented info classes.

The purpose of this project is to reverse and document the underlying structures for the following info classes:
 - `PowerRequestCreate`
 - `PlmPowerRequestCreate`
 - `PowerRequestAction`
 - `GetPowerRequestList`

## Modes

 - **DISPLAY** - does not allow dimming the display after a timeout.
 - **SYSTEM** - keeps the system awake, preventing automatic shutdown.
 - **AWAYMODE** - keeps the system awake for background processing while it appears to be sleeping. When explicitly requesting sleep, the computer merely turns off the display and sound.
 - **EXECUTION** - overrides Process Lifetime Management mechanisms; requires Windows 8 and higher.
 - **PERFBOOST** - unknown; requires Windows 8 and higher.
 - **ACTIVELOCKSCREEN** - unknown; only allowed for processes in an interactive session; requires Windows 10 RS1 and higher.

## Info classes

#### PowerRequestCreate

This information class captures the provided diagnostic reason and creates a PowerRequest kernel object, returning an exclusive handle. The reason can either be a simple message or a localized string resource in a DLL (for which you can also supply parameters). The request is logically assigned to the current process and is not active by default. A documented function [with the same name](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powercreaterequest) exposes the full functionality of this info class.

#### PlmPowerRequestCreate

This information class has the same parameters and works somewhat similar to the previous one but creates a Process Lifetime Management request. The resultant object does not reference the calling process; instead, the first action assigns one to it. Additionally, PLM requests do not show up in enumerations and only support the EXECUTION mode.

#### PowerRequestAction

This information class allows activating and deactivating required modes for a power request, one at a time. Although two documented functions ([PowerSetRequest](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powersetrequest) and [PowerClearRequest](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powerclearrequest)) partially expose its functionality, they only support DISPLAY, SYSTEM, AWAYMODE, and EXECUTION modes, blocking PERFBOOST and ACTIVELOCKSCREEN. Additionally, they do not allow supplying the target process handle for PLM requests (which requires PROCESS_SET_LIMITED_INFORMATION, by the way).

#### GetPowerRequestList

This information class is mainly used by powercfg and requires administrative rights. It enumerates all outstanding power requests, including those that are not currently affecting the system. Each entry contains the following information:

 - Active counter for each supported mode
 - Device name and description for requests created by device drivers
 - Image name in native format for user-mode creators
 - Process ID and service tag of the creator
 - Diagnostic context information (simple or localized reason message)
