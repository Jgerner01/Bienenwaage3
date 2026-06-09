Import("env")
import re, os

header_path = "src/build_number.h"
try:
    with open(header_path) as f:
        m = re.search(r"BUILD_NUMBER\s+(\d+)", f.read())
    num = int(m.group(1)) + 1 if m else 1
except Exception:
    num = 1

with open(header_path, "w") as f:
    f.write(f"#pragma once\n#define BUILD_NUMBER {num}\n")

print(f"*** Bienenwaage3 Build #{num} ***")
