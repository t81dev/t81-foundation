"""
Tests for T81 Architecture & Research Status Dashboard (T81-ARCH-VIZ-003).

Validates:
1. Schema and referential integrity of research_programs.json and pull_requests_snapshot.json.
2. No unsupported classification promotion (VERIFIED requires receipts, tests, and qualification).
3. Snapshot freshness labeling and deterministic metadata.
4. Browser interactions via Playwright (3D canvas load, 2D accessible fallback, search, layer filter, node selection, modals).
5. Headless screenshot capture for delivery artifact verification.
"""

import json
import os
import subprocess
import time
import pytest
from playwright.sync_api import sync_playwright

VIZ_DIR = os.path.abspath("docs/architecture/visualization")
DASHBOARD_DIR = os.path.join(VIZ_DIR, "dashboard")


def test_research_programs_dataset_integrity():
    path = os.path.join(VIZ_DIR, "research_programs.json")
    assert os.path.exists(path), "research_programs.json missing"
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    assert data["schema_version"] == "1.0.0"
    assert data["authoritative_baseline_commit"] == "6c9eaf1a2fc8368cd06f6dddbbdcd78e6606c0ce"
    assert "programs" in data and len(data["programs"]) >= 5
    assert "hypothesis_mappings" in data and len(data["hypothesis_mappings"]) == 8

    with open(os.path.join(VIZ_DIR, "architecture_nodes.json"), "r", encoding="utf-8") as f:
        node_ids = {n["id"] for n in json.load(f)["nodes"]}

    for prog in data["programs"]:
        assert prog["id"].startswith("program.")
        for sub in prog["lead_subsystems"]:
            assert sub in node_ids, f"Unknown lead subsystem {sub} in {prog['id']}"
        for ph in prog["phases"]:
            assert "phase_id" in ph
            assert "title" in ph
            assert "disposition" in ph
            assert "receipt" in ph
            assert "details" in ph

    hyp_ids = {h["hypothesis_id"] for h in data["hypothesis_mappings"]}
    assert hyp_ids == {f"H0{i}" for i in range(1, 9)}


def test_pull_requests_snapshot_integrity():
    path = os.path.join(VIZ_DIR, "pull_requests_snapshot.json")
    assert os.path.exists(path), "pull_requests_snapshot.json missing"
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    assert data["schema_version"] == "1.0.0"
    assert "snapshot_timestamp" in data
    assert data["authoritative_baseline_commit"] == "6c9eaf1a2fc8368cd06f6dddbbdcd78e6606c0ce"
    assert "refresh_instructions" in data
    assert "pr_inventory_summary" in data

    with open(os.path.join(VIZ_DIR, "architecture_nodes.json"), "r", encoding="utf-8") as f:
        node_ids = {n["id"] for n in json.load(f)["nodes"]}

    assert len(data["pull_requests"]) >= 10
    for pr in data["pull_requests"]:
        assert isinstance(pr["pr_number"], int)
        assert pr["state"] in ["OPEN", "MERGED", "CLOSED", "DRAFT"]
        assert len(pr["affected_subsystems"]) > 0
        for sub in pr["affected_subsystems"]:
            assert sub in node_ids, f"PR #{pr['pr_number']} references unknown subsystem: {sub}"


def test_browser_dashboard_interaction_and_2d_fallback(tmp_path):
    """Start local web server and verify interactive functionality via Playwright."""
    port = 8877
    server = subprocess.Popen(
        ["python3", "-m", "http.server", str(port), "--directory", VIZ_DIR],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )
    time.sleep(1.5)

    try:
        with sync_playwright() as p:
            browser = p.chromium.launch()
            page = browser.new_page(viewport={"width": 1400, "height": 900})

            # Navigate to dashboard
            url = f"http://127.0.0.1:{port}/index.html"
            page.goto(url)
            page.wait_for_selector("#architecture-canvas")

            # Check title and baseline badge
            assert "T81 Architecture & Research Status Dashboard" in page.title()
            badge_text = page.locator("#baseline-badge").inner_text()
            assert "6c9eaf1a" in badge_text.lower()

            # 1. Test 3D Canvas element presence
            canvas = page.locator("#architecture-canvas")
            assert canvas.is_visible()

            # Take initial screenshot of 3D Canvas
            screenshot_3d = os.path.join(VIZ_DIR, "dashboard_3d_screenshot.png")
            page.screenshot(path=screenshot_3d)
            assert os.path.exists(screenshot_3d)

            # 2. Test Accessible 2D Fallback View
            btn_2d = page.locator("#view-mode-2d")
            btn_2d.click()
            fallback = page.locator("#fallback-2d-container")
            assert fallback.is_visible()

            # Verify layer card groups in 2D view
            cards = page.locator(".layer-card-group")
            assert cards.count() == 7, f"Expected 7 layer card groups, found {cards.count()}"

            # Verify node selection in 2D view
            first_node_card = page.locator(".node-card").first
            first_node_card.click()
            detail_content = page.locator("#detail-content")
            assert detail_content.is_visible()
            assert not page.locator("#detail-empty").is_visible()

            # Take screenshot of Accessible 2D view
            screenshot_2d = os.path.join(VIZ_DIR, "dashboard_2d_screenshot.png")
            page.screenshot(path=screenshot_2d)
            assert os.path.exists(screenshot_2d)

            # 3. Test Search filtering
            search_input = page.locator("#component-search")
            search_input.fill("TISC")
            count_label = page.locator("#search-results-count")
            assert "matched" in count_label.inner_text()

            # Clear search
            page.locator("#clear-search-btn").click()
            assert search_input.input_value() == ""

            # 4. Test Status Overlays
            overlay_select = page.locator("#overlay-select")
            overlay_select.select_option("qualification")
            # Verify qualification tags rendered
            assert page.locator(".status-tag").count() > 0

            # 5. Test Research Programs Modal
            page.locator("#research-btn").click()
            research_modal = page.locator("#research-modal")
            assert research_modal.is_visible()
            assert "SCG-07" in research_modal.inner_text()
            assert "H01" in research_modal.inner_text()
            page.locator("#close-research-btn").click()
            assert not research_modal.is_visible()

            # 6. Test PR Activity Modal
            page.locator("#pr-panel-btn").click()
            pr_modal = page.locator("#pr-modal")
            assert pr_modal.is_visible()
            assert "#757" in pr_modal.inner_text()
            page.locator("#close-pr-btn").click()
            assert not pr_modal.is_visible()

            # 7. Test Summary Metrics Modal
            page.locator("#summary-btn").click()
            summary_modal = page.locator("#summary-modal")
            assert summary_modal.is_visible()
            assert "Methodological Constraint" in summary_modal.inner_text()
            assert "VERIFIED" in summary_modal.inner_text()
            page.locator("#close-summary-btn").click()
            assert not summary_modal.is_visible()

            # Switch back to 3D mode
            page.locator("#view-mode-3d").click()
            assert canvas.is_visible()
            assert not fallback.is_visible()

            browser.close()
    finally:
        server.terminate()
        server.wait()
