#!/bin/bash
#
# Public header discipline (R6, R8, KTD18).
#
# Fails on, in include/:
#   * a function-like macro
#   * the `inline` keyword
#   * a variadic (`...`) parameter
#   * a public function declaration with no availability annotation
#   * in a mirror header, a function whose preceding comment names neither the
#     AppKit selector it wraps nor carries the `syntonic-owned` tag
#
# A mirror header wraps one SDK header of the same stem, so every function in
# it has an AppKit counterpart. Non-mirror headers - listed in NON_MIRROR below
# - hold builders and helpers that have none.
#
# Usage: scripts/lint-headers.sh [header-or-directory ...]   (default: include)

set -euo pipefail

exec /usr/bin/env python3 - "$@" <<'PYTHON'
import os
import re
import sys

# Headers with no AppKit counterpart: builders, helpers, the umbrella (KTD18).
NON_MIRROR = {
    "syntonic.h",
    "ns_base.h",
    "ns_layout.h",
    "ns_menu_bar.h",
}

AVAILABILITY = re.compile(r"\bAPI_(?:AVAILABLE|DEPRECATED|UNAVAILABLE)\s*\(")
FUNCTION_MACRO = re.compile(r"^[ \t]*#[ \t]*define[ \t]+[A-Za-z_]\w*\(", re.M)
INLINE = re.compile(r"\binline\b")
# -[NSButton setTitle:] / +[NSColor labelColor] and friends.
SELECTOR = re.compile(r"[-+]\s*\[\s*NS[A-Za-z0-9_]*\s+[^\]]+\]")
SKIP_CHUNK = re.compile(
    r"^(typedef|struct|union|enum|extern|static_assert|_Static_assert)\b")


def split_code(text):
    """Yield (code_chunk, preceding_comment, line) per top-level declaration,
    with comments stripped out of the code but kept alongside it."""
    chunks = []
    code = []
    comment = []
    depth = 0
    line = 1
    start_line = None
    i = 0
    n = len(text)

    def emit():
        chunk = "".join(code).strip()
        if chunk:
            chunks.append((chunk, "\n".join(comment), start_line or line))

    while i < n:
        c = text[i]
        two = text[i:i + 2]
        if two == "/*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            comment.append(text[i:j])
            line += text.count("\n", i, j)
            i = j
            continue
        if two == "//":
            j = text.find("\n", i)
            j = n if j < 0 else j
            comment.append(text[i:j])
            i = j
            continue
        if c == "#" and (not code or code[-1] == "\n"):
            # Preprocessor lines are checked separately, on the raw text.
            j = i
            while j < n:
                j = text.find("\n", j)
                if j < 0:
                    j = n
                    break
                if text[j - 1] != "\\":
                    break
                j += 1
            line += text.count("\n", i, j)
            i = j
            continue
        if c == "\n":
            line += 1
        elif not c.isspace() and start_line is None:
            start_line = line
        if c == "{":
            # `extern "C" {` is transparent; a struct, union or enum body, or a
            # function body that should not be here at all, is not.
            if not re.search(r"\bextern\b", "".join(code)):
                depth += 1
            code.append(c)
            i += 1
            continue
        if c == "}":
            code.append(c)
            i += 1
            if depth > 0:
                depth -= 1
                if depth == 0:
                    emit()
                    comment = []
                    code = []
                    start_line = None
            continue
        if c == ";":
            if depth == 0:
                emit()
                comment = []
                code = []
                start_line = None
            else:
                code.append(c)
            i += 1
            continue
        code.append(c)
        i += 1
    emit()
    return chunks


def is_function_declaration(chunk):
    if SKIP_CHUNK.match(chunk):
        return False
    if "(" not in chunk or ")" not in chunk:
        return False
    # A function-pointer variable or a call would not appear at file scope in a
    # header; anything left with a parameter list is a declaration.
    return re.search(r"\b[A-Za-z_]\w*\s*\(", chunk) is not None


def check(path, problems):
    name = os.path.basename(path)
    mirror = name not in NON_MIRROR
    with open(path, "r", encoding="utf-8") as handle:
        text = handle.read()

    def fail(line, message):
        problems.append("%s:%d: %s" % (path, line, message))

    for match in FUNCTION_MACRO.finditer(text):
        fail(text.count("\n", 0, match.start()) + 1,
             "function-like macro in a public header (R6)")

    for chunk, comment, line in split_code(text):
        if INLINE.search(chunk):
            fail(line, "`inline` in a public header (R6)")
        if "..." in chunk:
            fail(line, "variadic parameter in a public header (R6)")
        if not is_function_declaration(chunk):
            continue
        summary = " ".join(chunk.split())[:70]
        if not AVAILABILITY.search(chunk):
            fail(line, "no availability annotation (R8): %s" % summary)
        if mirror and not (
                "syntonic-owned" in comment or SELECTOR.search(comment)):
            fail(line,
                 "mirror header function names no AppKit selector and is not "
                 "tagged syntonic-owned (KTD18): %s" % summary)


def main(argv):
    roots = argv[1:] or ["include"]
    headers = []
    for root in roots:
        if os.path.isdir(root):
            for base, _, names in os.walk(root):
                headers.extend(os.path.join(base, f)
                               for f in sorted(names) if f.endswith(".h"))
        else:
            headers.append(root)
    if not headers:
        print("lint-headers: no headers found", file=sys.stderr)
        return 1

    problems = []
    for header in sorted(headers):
        check(header, problems)
    for problem in problems:
        print(problem)
    print("lint-headers: %d header(s), %d problem(s)"
          % (len(headers), len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
PYTHON
