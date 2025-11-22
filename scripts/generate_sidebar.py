#!/usr/bin/env python3
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # repo root
DOCS = ROOT / "docs"
INCLUDE = DOCS / "_includes" / "sidebar.html"

SECTIONS = {
    "Specifications": DOCS / "spec",
    "RFCs": DOCS / "rfcs",
    "Guides": DOCS / "guides",
}

def collect_markdown_files(path):
    items = []
    for file in sorted(path.glob("*.md")):
        if file.name.lower() == "index.md":
            continue
        items.append(file)
    return items

def make_link(path):
    # Convert "docs/spec/t81-overview.md" → "/t81-foundation/spec/t81-overview.html"
    rel = path.relative_to(DOCS)
    rel_html = str(rel).replace(".md", ".html")
    return f"/t81-foundation/{rel_html}"

def generate_sidebar():
    html = []
    html.append('<nav class="sidebar-nav">')

    for section, path in SECTIONS.items():
        html.append(f"  <h3>{section}</h3>")
        html.append("  <ul>")
        files = collect_markdown_files(path)
        for f in files:
            name = f.stem
            link = make_link(f)
            html.append(f'    <li><a href="{link}">{name}</a></li>')
        html.append("  </ul>\n")

    html.append("</nav>")
    return "\n".join(html)

def main():
    sidebar = generate_sidebar()
    INCLUDE.write_text(sidebar)
    print("Sidebar generated at:", INCLUDE)

if __name__ == "__main__":
    main()
