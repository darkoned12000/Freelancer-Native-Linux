#!/usr/bin/env python3
"""Generate/maintain a dependency graph from src/**/*.vcxproj.

Outputs (relative to repo root by default):
  - linux/docs/vcxproj-dependency-graph.json
  - linux/docs/vcxproj-dependency-graph.md

The graph is derived from MSBuild ProjectReference includes and
AdditionalDependencies .lib tokens (excluding %(...) placeholders). It is a
build/reference hint graph, not a complete runtime/interface dependency model.
"""

from __future__ import annotations

import argparse
import glob
import json
import os
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict

MSB_NS = {"msb": "http://schemas.microsoft.com/developer/msbuild/2003"}


def _norm_token_name(tok: str) -> str:
    """Extract a dependency name from a project/lib token/path."""
    tok = tok.strip().strip('"').strip("'")
    tok = tok.replace("\\", "/")
    base = os.path.basename(tok)
    name, _ext = os.path.splitext(base)
    return name.strip()


def parse_vcxproj(path: str, src_root: str):
    name = os.path.splitext(os.path.basename(path))[0]
    rel = os.path.relpath(path, src_root).replace("\\", "/")
    project_refs: set[str] = set()
    additional_deps: set[str] = set()

    try:
        root = ET.parse(path).getroot()
    except Exception as e:  # noqa: BLE001 - tool script reports parse issues in output
        return name, {
            "path": rel,
            "project_references": [],
            "additional_dependencies": [],
            "all_dependencies": [],
            "error": str(e),
        }

    for pr in root.findall(".//msb:ProjectReference", MSB_NS):
        inc = pr.get("Include") or ""
        if not inc:
            continue
        ref = _norm_token_name(inc)
        if ref:
            project_refs.add(ref)

    for ad in root.findall(".//msb:AdditionalDependencies", MSB_NS):
        if not ad.text:
            continue
        # MSBuild AdditionalDependencies is semicolon-delimited (paths may be quoted).
        for raw in ad.text.split(";"):
            tok = raw.strip().strip('"').strip("'")
            if not tok or "%" in tok or not tok.lower().endswith(".lib"):
                continue
            dep = _norm_token_name(tok)
            if dep:
                additional_deps.add(dep)

    project_refs.discard(name)
    additional_deps.discard(name)
    all_deps = project_refs | additional_deps
    return name, {
        "path": rel,
        "project_references": sorted(project_refs),
        "additional_dependencies": sorted(additional_deps),
        "all_dependencies": sorted(all_deps),
    }


def _canonicalize_name(name: str, proj_case_map: dict[str, str]) -> str:
    return proj_case_map.get(name.lower(), name)


def _canonicalize_list(values: list[str], proj_case_map: dict[str, str]) -> list[str]:
    return sorted({_canonicalize_name(v, proj_case_map) for v in values if v})


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", default="src", help="Source root containing .vcxproj files (default: src)")
    ap.add_argument("--json-out", default="linux/docs/vcxproj-dependency-graph.json")
    ap.add_argument("--md-out", default="linux/docs/vcxproj-dependency-graph.md")
    args = ap.parse_args()

    vcx_files = sorted(glob.glob(os.path.join(args.src, "**", "*.vcxproj"), recursive=True))
    if not vcx_files:
        print(f"no .vcxproj files found under {args.src}", file=sys.stderr)
        return 1

    projects: dict[str, dict] = {}
    for path in vcx_files:
        name, info = parse_vcxproj(path, args.src)
        projects[name] = info

    # Canonicalize dependency names to known project names case-insensitively.
    proj_case_map = {n.lower(): n for n in projects}
    for n, info in projects.items():
        if "error" in info:
            continue
        info["project_references"] = _canonicalize_list(info.get("project_references", []), proj_case_map)
        info["additional_dependencies"] = _canonicalize_list(info.get("additional_dependencies", []), proj_case_map)
        info["all_dependencies"] = _canonicalize_list(info.get("all_dependencies", []), proj_case_map)

    rev = defaultdict(list)
    for n, info in projects.items():
        for r in info.get("all_dependencies", []):
            rev[r].append(n)
    for n in projects:
        projects[n]["depended_on_by"] = sorted(set(rev.get(n, [])))

    graph = {
        "generated_from": f"{args.src}/**/*.vcxproj",
        "project_count": len(projects),
        "projects": projects,
    }

    os.makedirs(os.path.dirname(args.json_out) or ".", exist_ok=True)
    os.makedirs(os.path.dirname(args.md_out) or ".", exist_ok=True)
    with open(args.json_out, "w", encoding="utf-8") as f:
        json.dump(graph, f, indent=2, sort_keys=True)
        f.write("\n")

    lines = [
        "# vcxproj Dependency Graph",
        "",
        f"Generated from `{args.src}/**/*.vcxproj` by `linux/tools/generate_vcxproj_dep_graph.py`.",
        "",
        "Sources: `<ProjectReference>` includes and `<AdditionalDependencies>` `.lib` tokens (excluding `%(...)` placeholders). "
        "Names are normalized (basename, path/macro/quote stripping for `.lib` tokens, semicolon-delimited parsing) and "
        "canonicalized to known project names case-insensitively where possible. This is a **build/reference hint graph**, "
        "not a complete runtime/interface or COM/DACOM dependency model.",
        "",
        "| Project | Path | Project refs | Additional deps (.lib) | All deps (union) | Depended on by |",
        "|---|---|---|---|---|---|",
    ]
    for n in sorted(projects.keys(), key=str.lower):
        info = projects[n]
        err = info.get("error")
        if err:
            lines.append(f"| `{n}` | `{info.get('path','')}` | (parse error) | (parse error) | (parse error) | - |")
            continue
        prefs = ", ".join(f"`{d}`" for d in info.get("project_references", [])) or "-"
        adeps = ", ".join(f"`{d}`" for d in info.get("additional_dependencies", [])) or "-"
        alldeps = ", ".join(f"`{d}`" for d in info.get("all_dependencies", [])) or "-"
        rdeps = ", ".join(f"`{d}`" for d in info.get("depended_on_by", [])) or "-"
        lines.append(f"| `{n}` | `{info.get('path','')}` | {prefs} | {adeps} | {alldeps} | {rdeps} |")

    with open(args.md_out, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    print(f"wrote {args.json_out} and {args.md_out} ({len(projects)} projects)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
