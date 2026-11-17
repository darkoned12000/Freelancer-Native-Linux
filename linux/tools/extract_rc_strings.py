#!/usr/bin/env python3
"""Extract string tables from .rc files into a Linux-readable C header."""
import re, sys, os, glob

def extract_strings(rc_path):
    strings = []
    with open(rc_path, 'r', errors='replace') as f:
        for line in f:
            # Match STRINGTABLE entries: ID, "text"
            m = re.match(r'\s*(\d+)\s*,\s*"(.*)"', line)
            if m:
                sid = int(m.group(1))
                text = m.group(2).replace('\\"', '"')
                strings.append((sid, text))
    return strings

def main():
    rc_files = sys.argv[1:] if len(sys.argv) > 1 else glob.glob('src/**/*.rc', recursive=True)
    all_strings = {}
    for rc in rc_files:
        strs = extract_strings(rc)
        if strs:
            all_strings[rc] = strs
    # Generate C header
    print('#pragma once')
    print('// Auto-generated string table from .rc files')
    print('#include <stddef.h>')
    print('')
    for rc, strs in all_strings.items():
        basename = os.path.basename(rc).replace('.', '_')
        print(f'// From {rc}')
        print(f'static const char* {basename}_strings[] = {{')
        for sid, text in strs:
            escaped = text.replace('\\', '\\\\').replace('"', '\\"')
            print(f'  /* {sid} */ "{escaped}",')
        print('};')
        print('')

if __name__ == '__main__':
    main()
