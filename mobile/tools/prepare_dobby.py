#!/usr/bin/env python3
"""Apply an ELF/PIC assembler portability fix to the pinned Dobby source.

Upstream's ARM64 closure bridge uses Mach-O @PAGE relocations unconditionally.
Android requires GOT relocations; code and calling convention are unchanged.
"""
import sys
from pathlib import Path

root = Path(sys.argv[1])
path = root / "source/TrampolineBridge/ClosureTrampolineBridge/arm64/closure_bridge_arm64.asm"
old = "adrp TMP_REG_0, cdecl(common_closure_bridge_handler)@PAGE\nadd TMP_REG_0, TMP_REG_0, cdecl(common_closure_bridge_handler)@PAGEOFF"
text = path.read_text()
if text.count(old) != 1:
    raise SystemExit("Pinned Dobby assembly changed; review before building")
replacement = """// SoraVynx portability change: use ELF PIC relocations on Android/Linux.
#if defined(__APPLE__)
""" + old + """
#else
adrp TMP_REG_0, :got:common_closure_bridge_handler
ldr TMP_REG_0, [TMP_REG_0, :got_lo12:common_closure_bridge_handler]
#endif"""
path.write_text(text.replace(old, replacement))
