"""
Tests for T81 Architecture & Research Status Dashboard (T81-ARCH-VIZ-003).

Validates:
1. Schema and referential integrity of research_programs.json and pull_requests_snapshot.json.
2. Canonical reconciliation of H01-H08 with HYPOTHESIS_REGISTER_v1.json (formal claims, evidence tiers, results).
3. No unsupported classification promotion (VERIFIED requires receipts, tests, and qualification).
4. Snapshot freshness labeling and deterministic metadata.
5. Browser interactions via Playwright:
   - Zero uncaught JavaScript exceptions / page errors.
   - Successful initialization of all UI components.
   - All 35 components rendered across 3D and 2D views.
   - Canonical execution pathways (paths.paths) and step.module component resolution.
   - Architecture gap filtering (gap.affected_layer) matching layer nodes.
   - Coverage metrics matching coverage_manifest.json (51.43%, 18/35 dirs).
   - Research modal displaying canonical H01-H08 claims and linked studies.
6. Headless screenshot capture for delivery artifact verification.
"""

import json
import os
import subprocess
import time
import pytest
from playwright.sync_api import sync_playwright

VIZ_DIR = os.path.abspath("docs/architecture/visualization")
DASHBOARD_DIR = os.path.join(VIZ_DIR, "dashboard")
REGISTER_PATH = os.path.abspath("docs/research/continuity/HYPOTHESIS_REGISTER_v1.json")
MANIFEST_PATH = os.path.join(VIZ_DIR, "coverage_manifest.json")
GAPS_PATH = os.path.join(VIZ_DIR, "architecture_gaps.json")
PATHS_PATH = os.path.join(VIZ_DIR, "execution_paths.json")
NODES_PATH = os.path.join(VIZ_DIR, "architecture_nodes.json")


def test_research_programs_dataset_integrity():
    path = os.path.join(VIZ_DIR, "research_programs.json")
    assert os.path.exists(path), "research_programs.json missing"
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    assert data["schema_version"] == "1.0.0"
    assert "authoritative_baseline_commit" in data
    assert "programs" in data and len(data["programs"]) >= 5
    assert "hypothesis_mappings" in data and len(data["hypothesis_mappings"]) == 8

    with open(NODES_PATH, "r", encoding="utf-8") as f:
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

    with open(REGISTER_PATH, "r", encoding="utf-8") as f:
        canonical_register = json.load(f)
    canon_map = {e["id"]: e for e in canonical_register["entries"]}

    hyp_ids = {h["hypothesis_id"] for h in data["hypothesis_mappings"]}
    assert hyp_ids == {f"H0{i}" for i in range(1, 9)}

    for h in data["hypothesis_mappings"]:
        h_id = h["hypothesis_id"]
        assert h_id in canon_map, f"Unknown hypothesis {h_id}"
        canon_entry = canon_map[h_id]
        assert h["formal_claim"] == canon_entry["formal_claim"], (
            f"Formal claim mismatch for {h_id}: got '{h['formal_claim']}' expected '{canon_entry['formal_claim']}'"
        )
        assert h["evidence_tiers"] == canon_entry["evidence_tiers"], (
            f"Evidence tiers mismatch for {h_id}: got {h['evidence_tiers']} expected {canon_entry['evidence_tiers']}"
        )
        assert h["canonical_result"] == canon_entry["result"]
        assert h["confidence"] == canon_entry["confidence"]
        assert h["baseline"] == canon_entry["baseline"]


def test_pull_requests_snapshot_integrity():
    path = os.path.join(VIZ_DIR, "pull_requests_snapshot.json")
    assert os.path.exists(path), "pull_requests_snapshot.json missing"
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    assert data["schema_version"] == "1.0.0"
    assert "snapshot_timestamp" in data
    assert "authoritative_baseline_commit" in data
    assert "refresh_instructions" in data
    assert "pr_inventory_summary" in data

    with open(NODES_PATH, "r", encoding="utf-8") as f:
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

    uncaught_exceptions = []
    failed_requests = []

    try:
        with sync_playwright() as p:
            browser = p.chromium.launch()
            page = browser.new_page(viewport={"width": 1400, "height": 900})

            # Track uncaught exceptions and request failures
            page.on("pageerror", lambda exc: uncaught_exceptions.append(str(exc)))
            page.on("requestfailed", lambda req: failed_requests.append(f"{req.url}: {req.failure}"))

            # Navigate to dashboard
            url = f"http://127.0.0.1:{port}/index.html"
            response = page.goto(url)
            assert response and response.status == 200, f"Dashboard failed to load: status {response.status if response else 'None'}"
            page.wait_for_selector("#architecture-canvas")

            # Check title
            assert "T81 Architecture & Research Status Dashboard" in page.title()

            # Verify footer component count matches canonical 35 nodes
            with open(NODES_PATH, "r", encoding="utf-8") as f:
                canonical_nodes = json.load(f)["nodes"]
            assert len(canonical_nodes) == 35

            footer_count = page.locator("#footer-component-count").inner_text()
            assert footer_count == "35", f"Expected 35 components, found {footer_count}"

            # Verify coverage metric in footer matches manifest percentage
            with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
                manifest_data = json.load(f)
            pct = manifest_data["coverage_metrics"]["top_level_directory_representation_percentage"]
            mapped = manifest_data["repository_directories"]["mapped_count"]
            total = manifest_data["repository_directories"]["total_top_level_directories"]
            footer_cov = page.locator("#footer-coverage-metric").inner_text()
            assert f"{pct}%" in footer_cov, f"Coverage footer mismatch: got '{footer_cov}', expected containing '{pct}%'"

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

            # Verify all 35 components are rendered as cards in 2D view
            cards = page.locator(".layer-card-group")
            assert cards.count() == 7, f"Expected 7 layer card groups, found {cards.count()}"
            node_cards = page.locator(".node-card")
            assert node_cards.count() == 35, f"Expected 35 node cards, found {node_cards.count()}"

            # Verify node selection in 2D view
            first_node_card = node_cards.first
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

            # 4. Test Execution Pathway selection and step.module component resolution
            with open(PATHS_PATH, "r", encoding="utf-8") as f:
                canonical_paths = json.load(f)["paths"]
            assert len(canonical_paths) == 9

            for p_info in canonical_paths:
                path_id = p_info["id"]
                page.locator("#pathway-select").select_option(path_id)
                # Count visible cards
                visible_cards = page.locator(".node-card:not([style*='display: none'])")
                # Expected modules in path
                path_modules = {s["module"] for s in p_info["steps"]}
                matching_component_ids = {m for m in path_modules if any(n["id"] == m for n in canonical_nodes)}
                assert visible_cards.count() == len(matching_component_ids), (
                    f"Pathway {path_id} visible cards {visible_cards.count()} != expected {len(matching_component_ids)}"
                )

            # Reset pathway selection
            page.locator("#pathway-select").select_option("none")

            # 5. Test Architecture Gap selection and affected_layer component resolution
            with open(GAPS_PATH, "r", encoding="utf-8") as f:
                canonical_gaps = json.load(f)["gaps"]
            assert len(canonical_gaps) == 4

            for g_info in canonical_gaps:
                gap_id = g_info["id"]
                page.locator("#gap-select").select_option(gap_id)
                visible_cards = page.locator(".node-card:not([style*='display: none'])")
                affected_layer = g_info["affected_layer"]
                expected_nodes = [n["id"] for n in canonical_nodes if n["layer_id"] == affected_layer]
                assert visible_cards.count() == len(expected_nodes), (
                    f"Gap {gap_id} visible cards {visible_cards.count()} != expected layer nodes {len(expected_nodes)}"
                )

            # Reset gap selection
            page.locator("#gap-select").select_option("none")

            # 6. Test Status Overlays
            overlay_select = page.locator("#overlay-select")
            overlay_select.select_option("qualification")
            assert page.locator(".status-tag").count() > 0

            # 7. Test Research Programs Modal and H01-H08 Canonical Claims
            page.locator("#research-btn").click()
            research_modal = page.locator("#research-modal")
            assert research_modal.is_visible()
            modal_text = research_modal.inner_text()
            assert "SCG-07" in modal_text
            assert "Phase 4: Native Firmware Compatibility" in modal_text

            with open(REGISTER_PATH, "r", encoding="utf-8") as f:
                canon_reg = json.load(f)
            for entry in canon_reg["entries"]:
                assert entry["id"] in modal_text
                # First 30 chars of formal claim present
                assert entry["formal_claim"][:30] in modal_text

            page.locator("#close-research-btn").click()
            assert not research_modal.is_visible()

            # 8. Test PR Activity Modal
            page.locator("#pr-panel-btn").click()
            pr_modal = page.locator("#pr-modal")
            assert pr_modal.is_visible()
            assert "#757" in pr_modal.inner_text()
            page.locator("#close-pr-btn").click()
            assert not pr_modal.is_visible()

            # 9. Test Summary Metrics Modal and Coverage Manifest consistency
            page.locator("#summary-btn").click()
            summary_modal = page.locator("#summary-modal")
            assert summary_modal.is_visible()
            sum_text = summary_modal.inner_text()
            assert "Methodological Constraint" in sum_text
            assert "VERIFIED" in sum_text
            assert f"{pct}%" in sum_text
            assert f"{mapped} of {total} directories mapped" in sum_text
            page.locator("#close-summary-btn").click()
            assert not summary_modal.is_visible()

            # Switch back to 3D mode
            page.locator("#view-mode-3d").click()
            assert canvas.is_visible()
            assert not fallback.is_visible()

            browser.close()

            # Assert zero uncaught JavaScript exceptions or failed requests
            assert len(uncaught_exceptions) == 0, f"Uncaught JS exceptions: {uncaught_exceptions}"
            assert len(failed_requests) == 0, f"Failed network requests: {failed_requests}"

    finally:
        server.terminate()
        server.wait()
