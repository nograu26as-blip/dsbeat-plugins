#!/bin/bash
# DS BEAT: Fix mingw-w64 headers for JUCE 8 cross-compilation.
# - Installs newer upstream headers missing from Ubuntu's mingw-w64 11.0.1
# - Patches incorrect declarations that break JUCE's Direct2D/DirectWrite code
# Run with sudo/root after (re)installing mingw-w64.
set -e

UPSTREAM="https://raw.githubusercontent.com/mingw-w64/mingw-w64/master/mingw-w64-headers/include"
SYSROOT="/usr/x86_64-w64-mingw32/include"
SHARED="/usr/share/mingw-w64/include"

echo "=== 1. Download missing D2D headers from upstream ==="
for h in d2d1_3.h d2d1_3helper.h d2d1_2.h d2d1_2helper.h d2d1effects_1.h d2d1effects_2.h d2d1svg.h; do
    if [ ! -f "$SYSROOT/$h" ]; then
        curl -sL --max-time 30 "$UPSTREAM/$h" -o "$SYSROOT/$h"
        echo "installed $h"
    else
        echo "already present: $h"
    fi
done

echo "=== 2. Newer uiautomationcore.h (has ISelectionProvider2) ==="
if ! grep -q "ISelectionProvider2" "$SHARED/uiautomationcore.h" 2>/dev/null; then
    [ -f "$SHARED/uiautomationcore.h.bak" ] || cp "$SHARED/uiautomationcore.h" "$SHARED/uiautomationcore.h.bak"
    curl -sL --max-time 30 "$UPSTREAM/uiautomationcore.h" -o "$SHARED/uiautomationcore.h"
    echo "installed uiautomationcore.h"
else
    echo "already has ISelectionProvider2"
fi

echo "=== 3. Newer dwrite_3.h (no bogus IDWriteFactory4::CreateCustomRenderingParams) ==="
if grep -q 'interface IDWriteFactory4' -A40 "$SHARED/dwrite_3.h" 2>/dev/null | grep -q "CreateCustomRenderingParams"; then
    NEED_DWRITE3=1
else
    # check via awk across the interface block
    if awk '/^interface IDWriteFactory4 \{/,/^};/' "$SHARED/dwrite_3.h" | grep -q "CreateCustomRenderingParams"; then
        NEED_DWRITE3=1
    else
        NEED_DWRITE3=0
    fi
fi
# Simpler: check if file still has the old local marker (no upstream comment)
if [ "$NEED_DWRITE3" = "1" ] || ! grep -q "DS BEAT" "$SHARED/dwrite_3.h"; then
    if ! grep -q "DS BEAT" "$SHARED/dwrite_3.h"; then
        [ -f "$SHARED/dwrite_3.h.bak" ] || cp "$SHARED/dwrite_3.h" "$SHARED/dwrite_3.h.bak"
        curl -sL --max-time 30 "$UPSTREAM/dwrite_3.h" -o "$SHARED/dwrite_3.h"
        echo "installed upstream dwrite_3.h"
    fi
fi

echo "=== 4. Patch dwrite_3.h: add d2d1.h include + remove bogus Factory3 method ==="
python3 - <<'EOF'
path = "/usr/share/mingw-w64/include/dwrite_3.h"
with open(path) as f:
    c = f.read()

if "#include <d2d1.h> /* DS BEAT" not in c:
    old = "#include <dwrite_2.h>"
    assert c.count(old) == 1
    c = c.replace(old, "#include <dwrite_2.h>\n#include <d2d1.h> /* DS BEAT: D2D_RECT_F used below */")

old = """    virtual HRESULT STDMETHODCALLTYPE CreateCustomRenderingParams(
        FLOAT gamma,
        FLOAT enhanced_contrast,
        FLOAT grayscale_enhanced_contrast,
        FLOAT cleartype_level,
        DWRITE_PIXEL_GEOMETRY pixel_geometry,
        DWRITE_RENDERING_MODE1 rendering_mode,
        DWRITE_GRID_FIT_MODE gridfit_mode,
        IDWriteRenderingParams3 **params) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateFontFaceReference_("""
new = """    /* DS BEAT: removed incorrect 8-param CreateCustomRenderingParams from
       IDWriteFactory3 (not in the real SDK; hid the 6-param base version) */

    virtual HRESULT STDMETHODCALLTYPE CreateFontFaceReference_("""
if old in c:
    c = c.replace(old, new)
    print("removed bogus Factory3 method")

with open(path, "w") as f:
    f.write(c)
EOF

echo "=== 5. Patch dwrite_2.h: remove bogus Factory2 method ==="
python3 - <<'EOF'
path = "/usr/share/mingw-w64/include/dwrite_2.h"
with open(path) as f:
    c = f.read()

old = """    virtual HRESULT STDMETHODCALLTYPE CreateCustomRenderingParams(
        FLOAT gamma,
        FLOAT contrast,
        FLOAT grayscalecontrast,
        FLOAT cleartypeLevel,
        DWRITE_PIXEL_GEOMETRY pixelGeometry,
        DWRITE_RENDERING_MODE renderingMode,
        DWRITE_GRID_FIT_MODE gridFitMode,
        IDWriteRenderingParams2 **params) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateGlyphRunAnalysis("""
new = """    /* DS BEAT: removed incorrect 8-param CreateCustomRenderingParams from
       IDWriteFactory2 (not in the real SDK; hid the 6-param base version) */

    virtual HRESULT STDMETHODCALLTYPE CreateGlyphRunAnalysis("""
if old in c:
    c = c.replace(old, new)
    print("removed bogus Factory2 method")

with open(path, "w") as f:
    f.write(c)
EOF

echo "=== 6. Patch dcommon.h: DEFINE_ENUM_FLAG_OPERATORS for DWRITE_GLYPH_IMAGE_FORMATS ==="
python3 - <<'EOF'
path = "/usr/share/mingw-w64/include/dcommon.h"
with open(path) as f:
    c = f.read()

if "DEFINE_ENUM_FLAG_OPERATORS(DWRITE_GLYPH_IMAGE_FORMATS)" not in c:
    old = "} DWRITE_GLYPH_IMAGE_FORMATS;"
    assert c.count(old) == 1
    c = c.replace(old, "} DWRITE_GLYPH_IMAGE_FORMATS;\nDEFINE_ENUM_FLAG_OPERATORS(DWRITE_GLYPH_IMAGE_FORMATS); /* DS BEAT */")
    print("patched dcommon.h")

with open(path, "w") as f:
    f.write(c)
EOF

echo "=== All mingw header fixes applied ==="
