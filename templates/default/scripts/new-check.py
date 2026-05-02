#!/usr/bin/env python3
"""Scaffold a new clang-tidy check.

Usage:
    new-check <kebab-case-name>

Examples:
    new-check avoid-strcpy
    new-check prefer-constexpr-constructor

Creates:
    src/checks/<stem>.hh
    src/checks/<stem>.cc
    test/<plugin>-<name>.cpp

Edits:
    src/module.cc          (adds #include + registerCheck line)
    src/CMakeLists.txt     (adds the new .cc to target_sources)

Plugin namespace, prefix, library target, and the file-naming convention
(`<ns>_<name>` vs. plain `<name>`) are detected from the existing project,
so the same script works in both the upstream maitai project and in any
project initialized from `nix flake init -t github:sielicki/maitai`.
"""

import re
import sys
from pathlib import Path


def die(msg: str) -> None:
    print(f"new-check: error: {msg}", file=sys.stderr)
    sys.exit(1)


def kebab_to_snake(s: str) -> str:
    return s.replace("-", "_")


def kebab_to_pascal(s: str) -> str:
    return "".join(part.capitalize() for part in s.split("-"))


def detect_namespace_and_prefix(module_cc_text: str, source: Path) -> tuple[str, str]:
    ns = re.search(r"^namespace (\w+)\s*\{", module_cc_text, re.M)
    if not ns:
        die(f"could not detect namespace from {source}")
    reg = re.search(
        r'CheckFactories\.registerCheck<\w+>\(\s*"([\w]+(?:-[\w]+)*?)-',
        module_cc_text,
    )
    if not reg:
        die(f"could not detect plugin prefix from {source}")
    return ns.group(1), reg.group(1)


def detect_cmake_target(cmake_text: str, source: Path) -> str:
    m = re.search(r"add_library\(\s*(\w+)\s+MODULE", cmake_text)
    if not m:
        die(f"could not detect cmake library target from {source}")
    return m.group(1)


def file_stem(checks_dir: Path, namespace: str, snake: str) -> str:
    existing = [f.stem for f in checks_dir.glob("*.hh")]
    uses_prefix = any(s.startswith(f"{namespace}_") for s in existing)
    return f"{namespace}_{snake}" if uses_prefix else snake


def header_contents(namespace: str, class_name: str) -> str:
    return f"""#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace {namespace} {{

class {class_name} : public clang::tidy::ClangTidyCheck {{
public:
  {class_name}(clang::StringRef Name, clang::tidy::ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {{}}

  void registerMatchers(clang::ast_matchers::MatchFinder *Finder) override;

  void check(const clang::ast_matchers::MatchFinder::MatchResult &Result) override;
}};

}} // namespace {namespace}
"""


def source_contents(header_basename: str, namespace: str, class_name: str) -> str:
    return f"""#include "{header_basename}"

using namespace clang::ast_matchers;

namespace {namespace} {{

auto {class_name}::registerMatchers(MatchFinder *Finder) -> void {{
  // TODO: Define an AST matcher and bind it.
  // Example: Finder->addMatcher(functionDecl().bind("fn"), this);
  (void)Finder;
}}

auto {class_name}::check(const MatchFinder::MatchResult &Result) -> void {{
  // TODO: Emit a diagnostic, optionally attaching FixItHints.
  (void)Result;
}}

}} // namespace {namespace}
"""


def test_contents(qualified_name: str, prefix: str) -> str:
    return f"""// RUN: %clang_tidy -checks='-*,{qualified_name}' -load %{prefix}_plugin %s -- > %t 2>&1 || true
// RUN: %FileCheck --allow-empty %s < %t

// TODO: replace the stub below with code that should trigger the check, then
// replace the negative directive with a positive one asserting that your
// warning fires. See test/{prefix}-example.cpp (or any existing test) for the
// canonical pattern.
void example() {{}}
// CHECK-NOT: warning
"""


def insert_into_module_cc(
    text: str, header_basename: str, class_name: str, qualified_name: str
) -> str:
    if header_basename in text:
        die(f"{header_basename} is already included in src/module.cc")
    if f'"{qualified_name}"' in text:
        die(f"check {qualified_name!r} is already registered in src/module.cc")

    quoted = re.findall(r'#include "([^"]+\.hh)"', text)
    if not quoted:
        die("src/module.cc has no quoted includes; cannot determine insertion point")
    new_quoted = sorted(set(quoted + [header_basename]))
    new_block = "".join(f'#include "{h}"\n' for h in new_quoted)
    text, n = re.subn(r'(?:#include "[^"]+\.hh"\n)+', new_block, text, count=1)
    if n != 1:
        die("failed to substitute include block in src/module.cc")

    last = None
    for m in re.finditer(r"^(\s*)CheckFactories\.registerCheck<.*?\);", text, re.M):
        last = m
    if last is None:
        die("could not find existing registerCheck line in src/module.cc")
    indent = last.group(1)
    insertion = (
        f'\n{indent}CheckFactories.registerCheck<{class_name}>("{qualified_name}");'
    )
    return text[: last.end()] + insertion + text[last.end() :]


def insert_into_cmake(text: str, target: str, source_basename: str) -> str:
    if source_basename in text:
        die(f"{source_basename} is already in src/CMakeLists.txt")

    pattern = re.compile(
        rf"(target_sources\(\s*{re.escape(target)}\s+PRIVATE)([^()]+)(\))"
    )
    m = pattern.search(text)
    if m is None:
        die(
            f"could not find `target_sources({target} PRIVATE ...)` in src/CMakeLists.txt"
        )
    body = m.group(2).rstrip()
    new_body = body + f"\n  {source_basename}\n"
    return text[: m.start(2)] + new_body + text[m.end(2) :]


def main() -> None:
    args = sys.argv[1:]
    if args in (["-h"], ["--help"]):
        print(__doc__)
        sys.exit(0)
    if len(args) != 1:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    name = args[0]
    if not re.fullmatch(r"[a-z][a-z0-9-]*[a-z0-9]", name):
        die(f"invalid name {name!r}; use lowercase kebab-case (e.g. avoid-strcpy)")

    snake = kebab_to_snake(name)
    class_name = f"{kebab_to_pascal(name)}Check"

    root = Path.cwd()
    module_cc = root / "src" / "module.cc"
    cmake = root / "src" / "CMakeLists.txt"
    if not module_cc.is_file() or not cmake.is_file():
        die(f"run from project root (expected {module_cc} and {cmake})")

    module_text = module_cc.read_text()
    cmake_text = cmake.read_text()
    namespace, prefix = detect_namespace_and_prefix(module_text, module_cc)
    target = detect_cmake_target(cmake_text, cmake)

    checks_dir = root / "src" / "checks"
    checks_dir.mkdir(exist_ok=True)
    stem = file_stem(checks_dir, namespace, snake)

    hh_path = checks_dir / f"{stem}.hh"
    cc_path = checks_dir / f"{stem}.cc"
    test_path = root / "test" / f"{prefix}-{name}.cpp"
    for p in (hh_path, cc_path, test_path):
        if p.exists():
            die(f"{p} already exists")

    qualified_name = f"{prefix}-{name}"

    hh_path.write_text(header_contents(namespace, class_name))
    cc_path.write_text(source_contents(hh_path.name, namespace, class_name))
    test_path.write_text(test_contents(qualified_name, prefix))

    module_cc.write_text(
        insert_into_module_cc(module_text, hh_path.name, class_name, qualified_name)
    )
    cmake.write_text(insert_into_cmake(cmake_text, target, f"checks/{cc_path.name}"))

    rel = lambda p: p.relative_to(root)
    print(f"Created {rel(hh_path)}")
    print(f"Created {rel(cc_path)}")
    print(f"Created {rel(test_path)}")
    print(f"Updated {rel(module_cc)}")
    print(f"Updated {rel(cmake)}")
    print()
    print(f"Next: implement the matcher in {rel(cc_path)},")
    print(f"      flesh out {rel(test_path)},")
    print(f"      then `nix build` to compile and run the LIT tests.")


if __name__ == "__main__":
    main()
