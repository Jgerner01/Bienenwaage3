Import("env")
import os

DATA_DIR = "data"
OUTPUT   = "src/web_content.h"

FILES = [
    ("index.html", "WEB_INDEX_HTML", "HTMLEOF"),
    ("style.css",  "WEB_STYLE_CSS",  "CSSEOF"),
    ("app.js",     "WEB_APP_JS",     "JSEOF"),
]

def needs_update():
    if not os.path.exists(OUTPUT):
        return True
    out_mtime = os.path.getmtime(OUTPUT)
    for name, _, _ in FILES:
        src = os.path.join(DATA_DIR, name)
        if os.path.exists(src) and os.path.getmtime(src) > out_mtime:
            return True
    return False

if not needs_update():
    print("*** web_content.h ist aktuell, kein Rebuild nötig ***")
else:
    out = (
        "// Bienenwaage3 – Web-Inhalte als PROGMEM (aus data/ generiert)\n"
        "// Automatisch eingebettet – kein separates LittleFS-Image nötig\n"
        "#pragma once\n"
        "#include <pgmspace.h>\n"
        "\n"
    )
    for name, var, delim in FILES:
        path = os.path.join(DATA_DIR, name)
        with open(path, "r", encoding="utf-8") as f:
            content = f.read()
        content = content.rstrip("\n") + "\n"
        out += f'static const char {var}[] PROGMEM = R"{delim}(\n'
        out += content
        out += f'){delim}";\n\n'

    with open(OUTPUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(out)
    print("*** web_content.h aus data/ neu generiert ***")
