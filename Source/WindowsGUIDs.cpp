// DS BEAT: Definitions for Windows GUIDs that are declared in MinGW-w64 headers
// (via DEFINE_GUID) but missing from the MinGW import libraries.
// Only compiled for MinGW cross-builds; MSVC gets these from the system libs.

#ifdef __MINGW32__
#include <guiddef.h>

// CLSID_D2D1Opacity: D2D1 built-in opacity effect
// Value from DEFINE_GUID in d2d1effects_2.h
extern "C" const GUID CLSID_D2D1Opacity =
    { 0x811d79a4, 0xde28, 0x4454, { 0x80, 0x94, 0xc6, 0x46, 0x85, 0xf8, 0xbd, 0x4c } };

// CLSID_SpVoice: SAPI default voice
// Value from DEFINE_GUID in sapi54.h
extern "C" const GUID CLSID_SpVoice =
    { 0x96749377, 0x3391, 0x11d2, { 0x9e, 0xe3, 0x00, 0xc0, 0x4f, 0x79, 0x73, 0x96 } };

#endif // __MINGW32__
