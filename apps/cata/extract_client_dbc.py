#!/usr/bin/env python3
"""Extract the DBFilesClient tables (.dbc/.db2) of a 4.3.4 or 5.4.8 client into one directory.

The world database builders (apps/cata/build_world_base.sh DBC_DIR, apps/mop/build_world_base.sh
DBC_DIR_548) read these files. Only the archives that carry DBFilesClient are opened, so the large art,
model, sound and world archives are not needed:

- misc.MPQ (5.4.8) with every Data/wow-update-base-<build>.MPQ applied in build order;
- Data/<locale>/locale-<locale>.MPQ with every Data/<locale>/wow-update-<locale>-<build>.MPQ applied in
  build order. Its files are written last, so the localized tables win.

Needs the StormLib shared library. Build the bundled copy with
  cmake -S deps/StormLib -B /tmp/stormlib -DBUILD_SHARED_LIBS=ON -DSTORM_USE_BUNDLED_LIBRARIES=ON
  cmake --build /tmp/stormlib
and pass --stormlib /tmp/stormlib/libstorm.so (or set STORMLIB).

  extract_client_dbc.py --client "<client dir>" --out <dbc dir> [--locale enGB]
"""

import argparse
import ctypes
import os
import re
import struct
import sys

MPQ_OPEN_READ_ONLY = 0x100


class FindData(ctypes.Structure):
    _fields_ = [("cFileName", ctypes.c_char * 1024), ("szPlainName", ctypes.c_char_p),
                ("dwHashIndex", ctypes.c_uint), ("dwBlockIndex", ctypes.c_uint), ("dwFileSize", ctypes.c_uint),
                ("dwFileFlags", ctypes.c_uint), ("dwCompSize", ctypes.c_uint), ("dwFileTimeLo", ctypes.c_uint),
                ("dwFileTimeHi", ctypes.c_uint), ("lcLocale", ctypes.c_uint)]


def load_stormlib(path):
    lib = ctypes.CDLL(path)
    handle = ctypes.c_void_p
    lib.SFileOpenArchive.argtypes = [ctypes.c_char_p, ctypes.c_uint, ctypes.c_uint, ctypes.POINTER(handle)]
    lib.SFileOpenPatchArchive.argtypes = [handle, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint]
    lib.SFileExtractFile.argtypes = [handle, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint]
    lib.SFileFindFirstFile.argtypes = [handle, ctypes.c_char_p, ctypes.POINTER(FindData), ctypes.c_char_p]
    lib.SFileFindFirstFile.restype = handle
    lib.SFileFindNextFile.argtypes = [handle, ctypes.POINTER(FindData)]
    lib.SFileFindClose.argtypes = [handle]
    lib.SFileCloseArchive.argtypes = [handle]
    for function in (lib.SFileOpenArchive, lib.SFileOpenPatchArchive, lib.SFileExtractFile, lib.SFileFindNextFile):
        function.restype = ctypes.c_bool
    return lib


def updates(directory, prefix):
    """wow-update-<prefix>-<build>.MPQ files of a directory, oldest build first."""
    pattern = re.compile(rf"wow-update-{re.escape(prefix)}-(\d+)\.MPQ", re.IGNORECASE)
    found = []
    for name in os.listdir(directory):
        match = pattern.fullmatch(name)
        if match:
            found.append((int(match.group(1)), os.path.join(directory, name)))
    return [path for _, path in sorted(found)]


def extract(lib, archive, patches, out):
    mpq = ctypes.c_void_p()
    if not lib.SFileOpenArchive(archive.encode(), 0, MPQ_OPEN_READ_ONLY, ctypes.byref(mpq)):
        raise RuntimeError(f"cannot open {archive}")
    try:
        for patch in patches:
            # a NULL prefix lets StormLib detect the patch prefix (base, the locale or none)
            if not lib.SFileOpenPatchArchive(mpq, patch.encode(), None, 0):
                raise RuntimeError(f"cannot apply {patch}")
        names = set()
        found = FindData()
        search = lib.SFileFindFirstFile(mpq, b"DBFilesClient\\*", ctypes.byref(found), None)
        if search:
            while True:
                names.add(found.cFileName.decode("utf-8", "replace"))
                if not lib.SFileFindNextFile(search, ctypes.byref(found)):
                    break
            lib.SFileFindClose(search)
        written = missing = 0
        for name in sorted(names):
            if not name.lower().endswith((".dbc", ".db2")):
                continue
            # listfile entries a patch removed (and the client's *_internal names) do not extract
            if lib.SFileExtractFile(mpq, name.encode(), os.path.join(out, name.split("\\")[-1]).encode(), 0):
                written += 1
            else:
                missing += 1
        return written, missing
    finally:
        lib.SFileCloseArchive(mpq)


def client_build(out):
    """Build number in the WDB2 header of Item.db2, when present."""
    path = os.path.join(out, "Item.db2")
    if not os.path.exists(path):
        return None
    with open(path, "rb") as handle:
        header = handle.read(32)
    return struct.unpack_from("<I", header, 24)[0] if header[:4] == b"WDB2" else None


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--client", required=True, help="client directory (the one holding Data/)")
    parser.add_argument("--out", required=True, help="directory the .dbc/.db2 files are written to")
    parser.add_argument("--locale", default="enGB")
    parser.add_argument("--stormlib", default=os.environ.get("STORMLIB", "libstorm.so"))
    args = parser.parse_args()
    if not re.fullmatch(r"[a-z]{2}[A-Z]{2}", args.locale):
        parser.error(f"invalid locale: {args.locale!r}")

    data = os.path.join(args.client, "Data")
    locale_dir = os.path.join(data, args.locale)
    locale_mpq = os.path.join(locale_dir, f"locale-{args.locale}.MPQ")
    if not os.path.exists(locale_mpq):
        parser.error(f"{locale_mpq} not found")
    lib = load_stormlib(args.stormlib)
    os.makedirs(args.out, exist_ok=True)

    misc = os.path.join(data, "misc.MPQ")
    if os.path.exists(misc):
        written, missing = extract(lib, misc, updates(data, "base"), args.out)
        print(f"misc.MPQ: {written} files ({missing} listed but absent)")
    written, missing = extract(lib, locale_mpq, updates(locale_dir, args.locale), args.out)
    print(f"locale-{args.locale}.MPQ: {written} files ({missing} listed but absent)")
    build = client_build(args.out)
    if build:
        print(f"Item.db2 build: {build}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
