"""Reject debug dependencies and smoke-test the actual Windows plugin entry points."""
import ctypes
import os
from pathlib import Path
import struct
import sys

RATES = (250, 500, 750, 1000, 1250, 1500)

def inspect(path):
    data = path.read_bytes()
    def u16(offset): return struct.unpack_from("<H", data, offset)[0]
    def u32(offset): return struct.unpack_from("<I", data, offset)[0]
    pe = u32(60)
    if data[:2] != b"MZ" or data[pe:pe+4] != b"PE\0\0":
        raise RuntimeError(f"{path}: invalid PE")
    if u16(pe+4) != 0x8664:
        raise RuntimeError(f"{path}: expected Windows x64")
    optional = pe+24
    if u16(optional) != 0x20b:
        raise RuntimeError(f"{path}: expected PE32+")
    sections = optional + u16(pe+20)
    def offset(rva):
        for index in range(u16(pe+6)):
            section = sections + index*40
            va, raw_size, raw_offset = struct.unpack_from("<III", data, section+12)
            if va <= rva < va + max(u32(section+8), raw_size):
                return raw_offset + rva-va
        raise RuntimeError(f"{path}: unmapped RVA {rva:x}")
    def string(pos): return data[pos:data.index(b"\0", pos)].decode("ascii")
    imports = []
    descriptor = offset(u32(optional+112+8))
    while u32(descriptor+12):
        imports.append(string(offset(u32(descriptor+12))))
        descriptor += 20
    debug = {"msvcp140d.dll", "vcruntime140d.dll", "vcruntime140_1d.dll", "ucrtbased.dll"}
    if any(name.lower() in debug for name in imports):
        raise RuntimeError(f"{path}: debug runtime dependency: {imports}")
    return imports

def main():
    plugin_dir, sdk_dir = (Path(arg).resolve() for arg in sys.argv[1:3])
    if len(list(plugin_dir.glob("dmx_512_analyzer_*.dll"))) != len(RATES):
        raise RuntimeError("Expected exactly six DLLs")
    sdk_cookie = os.add_dll_directory(str(sdk_dir))
    runtime = ctypes.WinDLL(str(sdk_dir / "Analyzer.dll"))
    for rate in RATES:
        path = plugin_dir / f"dmx_512_analyzer_{rate}.dll"
        imports = inspect(path)
        dll = ctypes.CDLL(str(path))
        dll.GetAnalyzerName.restype = ctypes.c_char_p
        expected = f"DMX-512 ({rate} kbaud)".encode()
        if dll.GetAnalyzerName() != expected:
            raise RuntimeError(f"{path}: incorrect analyzer name")
        dll.CreateAnalyzer.restype = ctypes.c_void_p
        dll.DestroyAnalyzer.argtypes = [ctypes.c_void_p]
        analyzer = dll.CreateAnalyzer()
        if not analyzer:
            raise RuntimeError(f"{path}: CreateAnalyzer returned null")
        dll.DestroyAnalyzer(analyzer)
        print(f"{path.name}: x64, release imports, load/create/destroy passed ({', '.join(imports)})")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
