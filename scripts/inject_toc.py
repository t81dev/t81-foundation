#!/usr/bin/env python3
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"

TOC_START = "<!-- T81-TOC:BEGIN -->"
TOC_END = "<!-- T81-TOC:END -->"

EXCLUDE = {
    "index.md",
    "README.md"
}

def generate_toc(content):
    headings = []
    for line in content.split("\n"):
        m = re.match(r"^(#{1,4})\s+(.*)", line)
        if not m:
            continue
        level = len(m.group(1))
        title = m.group(2).strip()

        # GitHub-style anchor
        anchor = (
            title.lower()
            .replace(" ", "-")
            .replace("/", "")
            .replace("(", "")
            .replace(")", "")
            .replace(".", "")
            .replace(",", "")
            .replace(":", "")
            .replace(";", "")
        )

        headings.append((level, title, anchor))

    if not headings:
        return None

    toc = [TOC_START, ""]
    toc.append("## Table of Contents\n")

    for lvl, title, anchor in headings:
        indent = "  " * (lvl - 1)
        toc.append(f"{indent}- [{title}](#{anchor})")

    toc.append("")
    toc.append(TOC_END)
    toc.append("")
    return "\n".join(toc)


def inject_toc(path):
    text = path.read_text()

    # Remove previous TOC if present
    if TOC_START in text and TOC_END in text:
        pattern = re.compile(
            f"{re.escape(TOC_START)}.*?{re.escape(TOC_END)}\n?",
            re.DOTALL,
        )
        text = pattern.sub("", text)

    toc = generate_toc(text)
    if toc is None:
        return False

    # Insert TOC after first H1
    lines = text.split("\n")
    new_lines = []
    inserted = False

    for i, line in enumerate(lines):
        new_lines.append(line)
        if not inserted and line.startswith("# "):
            new_lines.append("")
            new_lines.append(toc)
            inserted = True

    path.write_text("\n".join(new_lines))
    return True


def walk_docs():
    for md in DOCS.rglob("*.md"):
        if md.name in EXCLUDE:
            continue
        inject_toc(md)


if __name__ == "__main__":
    walk_docs()
    print("TOC injection complete.")
