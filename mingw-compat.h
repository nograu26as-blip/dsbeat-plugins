// DS BEAT MinGW cross-compile compatibility header.
// Force-included via the toolchain file for Windows cross-builds only.
// Fills small gaps in mingw-w64 headers vs. the Windows SDK.
// All values below are stable Microsoft constants.

#pragma once

// --- UI Automation attribute IDs (MS UIAutomationCoreApi.h) ---
#ifndef UIA_IsReadOnlyAttributeId
#define UIA_IsReadOnlyAttributeId 10022
#endif

#ifndef UIA_CaretPositionAttributeId
#define UIA_CaretPositionAttributeId 10019
#endif

// --- CaretPosition enum (MS UIAutomationCore.h) ---
#ifndef DSBEAT_CARETPOSITION_DEFINED
#define DSBEAT_CARETPOSITION_DEFINED
typedef enum CaretPosition
{
    CaretPosition_Unknown         = 0,
    CaretPosition_EndOfLine       = 1,
    CaretPosition_BeginningOfLine = 2
} CaretPosition;
#endif

// --- D2D1 saturation effect property (MS d2d1effects.h) ---
#ifndef D2D1_SATURATION_PROP_SATURATION
#define D2D1_SATURATION_PROP_SATURATION 0
#endif

// --- Windows Media error codes (nserror.h not transitively included on MinGW) ---
#include <nserror.h>

// --- SAL annotation __callback (empty in MS SDK, missing in mingw-w64) ---
#ifndef __callback
#define __callback
#endif

// --- Missing GUID definitions (declared via DEFINE_GUID in headers, but not in MinGW libs) ---
// NOTE: The actual definitions are in Source/WindowsGUIDs.cpp (a .cpp file avoids
// multiple-definition and linkage issues that occur when defining them in a header).
#include <guiddef.h> // for GUID struct
