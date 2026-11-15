# T81 Architecture & Research Status Dashboard Guide (`T81-ARCH-VIZ-003`)

**Document ID:** `T81-ARCH-VIZ-003-GUIDE-R2`
**Authoritative Baseline:** `8e6918fa98e19ee1476f91c97e82989b051a02e7` (`origin/main`, post-PR #759)
**Status:** Corrected & Verified under Pinned Baseline
**Audience:** Maintainers, Researchers, System Integrators, and External Auditors

---

## 1. Overview & Architecture

The **T81 Architecture & Research Status Dashboard** is an evidence-grounded, browser-based user interface extending the machine-readable dataset foundation created in `T81-ARCH-VIZ-001` and `T81-ARCH-VIZ-002`, combined with the diagnostic audit framework established in `T81-ARCH-ENG-002`.

It provides an interactive interface to explore:
1. **Repository Components & Structure:** All 35 architectural components across the 7 established architectural layers.
2. **Relationships & Interfaces:** 32 technical dependency edges, 6 public subsystem interfaces, and 9 statically supported execution pathways (rendered via canonical `paths` and `step.module`).
3. **Evidence Classifications:** Real-time inspection of source code files, CMake test targets, and formal scientific receipts (`VERIFIED`, `STATICALLY_SUPPORTED`, `UNVERIFIED`, `NOT_MAPPED`).
4. **Independent Status Overlays:** Multi-dimensional tracking separating Implementation Maturity, Research Qualification, and Engineering Activity.
5. **Research Program Tracking:** Direct traceability to SCG-01..08, TernaryOS ARCH-ENG-002 Phases 1..4, TISC v1.1.0/v2.0, CanonFS, Hardware/RTL feasibility, and empirical hypothesis register H01–H08.
6. **Pull Request Subsystem Activity:** Deterministic tracking of open and recently merged pull requests affecting specific subsystems.

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                       docs/architecture/visualization/                          │
│                                                                                 │
│   architecture_layers.json  ───┐                                                │
│   architecture_nodes.json   ───┼──► [scripts/ci/validate_architecture_         │
│   architecture_edges.json   ───┤     visualization.py]                          │
│   component_evidence_index  ───┤      │ (Fail-Closed Semantic & Drift Checks)   │
│   research_programs.json    ───┤      ▼                                         │
│   pull_requests_snapshot    ───┴──► [Interactive Web Dashboard]                │
│                                     ├── 3D WebGL / Isometric Canvas Engine     │
│                                     ├── Accessible 2D Structural Grid Fallback  │
│                                     ├── Multi-Overlay Status Discriminator      │
│                                     └── Component Detail & Modals               │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Local Launch & Requirements

The dashboard requires **no external dependencies**, build steps, or remote network access. It is entirely self-contained within vanilla HTML5, CSS3, and modern standard JavaScript (ES2022).

### 2.1 Quick Launch (Local HTTP Server)

From the repository root:
```bash
# Launch a lightweight static server pointing to the visualization directory
python3 -m http.server 8080 --directory docs/architecture/visualization
```
Then open your browser to:
```
http://localhost:8080/index.html
```

Alternatively, if serving from repo root:
```bash
python3 -m http.server 8080
# Navigate to: http://localhost:8080/docs/architecture/visualization/
```

### 2.2 Browser Support
- **Chrome / Chromium / Edge:** Version 100+ (fully supported in both 3D and 2D).
- **Firefox:** Version 100+ (fully supported).
- **Safari:** Version 15+ (fully supported).
- **Headless / CI Browsers:** Tested and verified via Playwright Chromium.

---

## 3. User Interface & Navigation

### 3.1 3D Canvas Navigation
The 3D canvas renders each of the 7 layers at elevation levels defined in `visualization_layout.json` ($y \in [0, 600]$), using an isometric perspective projection.
- **Orbit (Rotate Pitch/Yaw):** Left Click + Drag on the canvas.
- **Pan (Translate View):** Right Click + Drag on the canvas.
- **Zoom (Scale):** Mouse Scroll Wheel or trackpad pinch.
- **Select Component:** Left Click directly on any node disk. The camera view maintains focus and opens the right detail drawer.
- **Reset Camera:** Click the **"Reset View"** button on the bottom left overlay.

### 3.2 Accessible 2D Structural Fallback
Click the **"Accessible 2D"** toggle in the top header.
- **Full Parity:** The 2D view presents every component grouped under its architectural layer card.
- **Keyboard Navigation:** Full keyboard focus management (`Tab`, `Shift+Tab`, `Enter`, `Space` to select).
- **Substantive Equivalence:** Shows status badges, categories, descriptions, and opens the exact same detailed evidence inspector.
- **Automatic Fallback:** If WebGL or 2D canvas context initialization fails or is disabled, the dashboard automatically transitions to the 2D structural view.

### 3.3 Layer Visibility & Filtering
- **Layer Toggles:** Click any layer in the left sidebar to toggle visibility on/off in both 3D and 2D modes.
- **Toggle All / None:** Use the quick buttons in the sidebar.
- **Evidence Filter Chips:** Filter components by evidence status (`All`, `Verified`, `Static`, `Unverified`, `Not Mapped`).
- **Text Search:** Type in the component search box to match names, IDs, descriptions, source files, or public interfaces.

### 3.4 Execution Pathways & Architecture Gaps
- **Execution Pathway Selector:** Highlights canonical end-to-end sequences (e.g., `01_bigint_add.trace`, `08_qemu_boot`).
- **Architecture Gap Selector:** Highlights documented architectural gaps (e.g., `gap.01_cpa006_parity_mismatch`).

---

## 4. Evidence Overlays & Semantic Dimensions

The dashboard provides four **mutually independent status overlays** selectable via the top header dropdown:

| Overlay | Dimensions / Values | Semantic Definition |
| :--- | :--- | :--- |
| **Evidence Classification** | `VERIFIED`<br>`STATICALLY_SUPPORTED`<br>`UNVERIFIED`<br>`NOT_MAPPED` | Machine-checkable evidence baseline defined in ARCH-ENG-002: formal receipts, tests, and qualification. |
| **Implementation Maturity** | `IMPLEMENTED`<br>`EXPERIMENTAL`<br>`PROPOSED`<br>`UNKNOWN` | Software presence in the codebase (compiled source vs research prototypes vs specifications). |
| **Research Qualification** | `QUALIFIED`<br>`QUALIFIED_WITH_LIMITATIONS`<br>`BLOCKED`<br>`NOT EVALUATED` | Formal scientific qualification status under an independent mathematical oracle. |
| **Engineering Activity** | `ACTIVE PR`<br>`MERGED MILESTONE`<br>`OPEN BLOCKER`<br>`NO RECORDED ACTIVITY` | Current pull request involvement from repository metadata. |

### Crucial Semantic Distinction
A component may simultaneously have:
- `IMPLEMENTED` implementation maturity (fully compiled C++ source code).
- `STATICALLY_SUPPORTED` evidence status (dedicated automated unit tests passing in CI).
- `UNVERIFIED` research qualification (no formal scientific qualification receipt).

**Never conflate test presence with independent scientific qualification.**

---

## 5. Research Programs & Hypothesis Integration

Click **"Research Programs"** in the top header to inspect the comprehensive program status modal.

### 5.1 SCG-01 through SCG-08 Boundaries
- **SCG-07 Phase 6:** Executive disposition is `QUALIFIED WITH LIMITATIONS`. All reproducers F1–F7 pass unconditionally against an independent base-$10^9$ exact integer oracle. F3 Karatsuba intermediate carry-loss invariance is proven congruent to $0 \pmod{3^{54}}$ under canonical modular residue semantics.
- **SCG-08 Phase 0:** Charter integrated via PR #753 (`0bd0817e`). Derives 3-state carry Markov chain ($E[c_{\text{active}}] = N/4$).
- **SCG-08 Phase 1 Contract:** Standardizes signed $B$-bit domain $[-2^{B-1}, 2^{B-1}-1]$. **Implementation authorization (Gate 1) and full campaign execution (Gate 2) remain strictly withheld.**

### 5.2 TernaryOS & ARCH-ENG-002 Boundaries
- **Phase 1 (PR #755):** Audited `((COMPLETED_TESTS++))` arithmetic defect under `set -e`.
- **Phase 2 (PR #756):** Consolidated QEMU verifier in Python with fail-closed QMP sockets; 11/11 mock tests pass; real QEMU halts and safely rejects failed attempts.
- **Phase 3 (PR #757):** Distinguishes stale tracked EFI probe (5,120 bytes) from working fresh EFI shell (75,264 bytes). Firmware diagnostic barriers prevent native 5-check qualification; native verification remains NOT QUALIFIED.
- **Phase 4 (PR #758):** Evaluated 2-firmware / 2-launch matrix across original and matched configurations; fail-closed verifier confirmed rejecting 4/4 rows (exit status 1); stage-0 TRNG query unsupported. Matched debug launch reaches real shell but retains firmware errors. **All four native configurations remain failed at 2/5; native 5-check verification remains NOT QUALIFIED.**

### 5.3 Hypothesis Register H01–H08 Dispositions & Linked Studies
All eight empirical hypotheses preserve their exact formal claims, source citations, evidence tiers, and canonical results from `docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`:
- **H01 (Tiers B, D):** "Equal-information ternary storage can have lower total physical cost than binary storage under stated device and overhead assumptions." Linked historical finding: software scalar tryte addition incurred 10.6x instruction overhead compared to native 64-bit Boolean addition (closed for scalar software; multi-precision re-evaluated in SCG-08).
- **H02 (Tiers A, B):** "Balanced ternary can improve a specified signed arithmetic or rounding task at equal semantics and cost accounting." Linked findings: SCG-07 qualified arithmetic primitives with limitations; SCG-08 derived 3-state carry Markov chain (1/4 non-zero carry occurrence vs 1/2 in binary); Tier 2 benchmark preregistered.
- **H03 (Tiers B, D):** "Grouping four trits as base-81 can provide a workload-specific interface or execution benefit beyond equivalent binary packing." Linked status: Base-81 grouping established; execution advantage unverified.
- **H04 (Tier B):** "An explicitly defined adaptive ternary representation policy can beat fixed ternary and an equally capable adaptive binary policy on a specified workload." Linked refinements: H04-R01 specification; EXP-H04-R03 Candidate A correctness and weak-reference wire-frame retention verified (PR #727); Phase C performance measurement frozen.
- **H05 (Tier B):** "A specified three-state primitive can reduce circuit or system cost against an equivalent Boolean implementation." Linked status: Verilog RTL simulation models 2-bit-per-trit dot product; comparative Boolean synthesis unperformed.
- **H06 (Tiers B, C):** "Ternary weights or activations can improve accuracy-constrained inference versus equally optimized binary low-bit alternatives." Linked status: Prior reports exist; independent accuracy-constrained benchmarks unperformed.
- **H07 (Tiers B, C):** "Representation-independent governed execution can improve a defined auditability property." Linked status: Axion PolicyEngine active; security certification pending independent protocol completion.
- **H08 (Tiers C, E):** "AI-assisted iteration improves research trustworthiness or discovery under an independently auditable process." Linked status: Audited via continuous session handoffs, negative result preservation, and mathematical oracle qualification.

---

## 6. Pull Request Snapshot & Refresh Procedure

The PR activity panel is driven deterministically by `docs/architecture/visualization/pull_requests_snapshot.json`.

### 6.1 Data Provenance
- Snapshot records cataloged pull requests mapped to affected subsystems.
- Embeds exact `base_sha`, `head_sha`, and `merge_commit`.
- Displays `snapshot_timestamp` and `authoritative_baseline_commit`.

### 6.2 Refresh Procedure
When refreshing PR status:
1. Fetch current `origin/main` and open PR inventory:
   ```bash
   git fetch origin
   git log -n 10 --oneline origin/main
   ```
2. Update `pull_requests_snapshot.json` with new PR numbers, affected subsystem IDs, and timestamps.
3. Validate schema and referential integrity:
   ```bash
   python3 scripts/ci/validate_architecture_visualization.py --drift-check
   pytest tests/ci/test_architecture_dashboard.py
   ```

---

## 7. Known Limitations & Strict Boundaries

1. **No System Completion Percentage:** The dashboard strictly does not calculate or display an overall "project completion %". Such metrics are scientifically invalid because software existence, test coverage, and formal qualification operate across independent dimensions.
2. **Local Browser Isolation:** The dashboard operates offline from static JSON files and does not issue external network requests.
3. **No Dynamic Execution Verification:** Viewing a component in the dashboard does not imply that its C++ code has executed in your browser session.
4. **Firmware Qualification Boundary:** Real QEMU execution of TernaryOS is currently diagnosed under PR #757 and PR #758 (all four matrix configurations failed at 2/5) and must not be represented as an accepted native pass.
