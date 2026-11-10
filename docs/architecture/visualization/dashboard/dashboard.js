/**
 * T81 Architecture & Research Status Dashboard (T81-ARCH-VIZ-003)
 * Interactive 3D Canvas + Accessible 2D Fallback + Evidence Overlays
 */

(function () {
  'use strict';

  // Global State
  const state = {
    layers: [],
    nodes: [],
    edges: [],
    interfaces: [],
    gaps: [],
    executionPaths: [],
    evidenceIndex: null,
    layout: null,
    coverage: null,
    researchPrograms: null,
    prSnapshot: null,

    // UI View State
    viewMode: '3d', // '3d' | '2d'
    activeOverlay: 'evidence', // 'evidence' | 'maturity' | 'qualification' | 'activity'
    activeEvidenceFilter: 'ALL',
    selectedNodeId: null,
    searchQuery: '',
    visibleLayers: new Set(),
    activePathwayId: 'none',
    activeGapId: 'none',

    // Canvas / 3D State
    camera: {
      rotX: 0.45,
      rotY: -0.55,
      zoom: 0.55,
      panX: 0,
      panY: 50
    },
    isDragging: false,
    dragButton: 0, // 0: left (orbit), 2: right (pan)
    lastMouseX: 0,
    lastMouseY: 0
  };

  // Node Lookups
  let nodesById = new Map();
  let edgesBySource = new Map();
  let edgesByTarget = new Map();
  let evidenceByComponentId = new Map();
  let prsBySubsystemId = new Map();

  // Document Elements
  const els = {};

  document.addEventListener('DOMContentLoaded', init);

  async function init() {
    cacheElements();
    setupEventListeners();
    await loadDatasets();
    initUI();
    render();
  }

  function cacheElements() {
    els.viewMode3D = document.getElementById('view-mode-3d');
    els.viewMode2D = document.getElementById('view-mode-2d');
    els.overlaySelect = document.getElementById('overlay-select');
    els.summaryBtn = document.getElementById('summary-btn');
    els.researchBtn = document.getElementById('research-btn');
    els.prPanelBtn = document.getElementById('pr-panel-btn');
    els.componentSearch = document.getElementById('component-search');
    els.clearSearchBtn = document.getElementById('clear-search-btn');
    els.searchResultsCount = document.getElementById('search-results-count');
    els.evidenceFilterChips = document.getElementById('evidence-filter-chips');
    els.layersList = document.getElementById('layers-list');
    els.btnLayersAll = document.getElementById('btn-layers-all');
    els.btnLayersNone = document.getElementById('btn-layers-none');
    els.pathwaySelect = document.getElementById('pathway-select');
    els.gapSelect = document.getElementById('gap-select');
    els.canvasContainer = document.getElementById('canvas-container');
    els.architectureCanvas = document.getElementById('architecture-canvas');
    els.resetCameraBtn = document.getElementById('reset-camera-btn');
    els.fallback2DContainer = document.getElementById('fallback-2d-container');
    els.fallbackGrid = document.getElementById('fallback-grid');
    els.detailTitle = document.getElementById('detail-title');
    els.closeDetailBtn = document.getElementById('close-detail-btn');
    els.detailEmpty = document.getElementById('detail-empty');
    els.detailContent = document.getElementById('detail-content');
    els.summaryModal = document.getElementById('summary-modal');
    els.closeSummaryBtn = document.getElementById('close-summary-btn');
    els.summaryModalBody = document.getElementById('summary-modal-body');
    els.researchModal = document.getElementById('research-modal');
    els.closeResearchBtn = document.getElementById('close-research-btn');
    els.researchModalBody = document.getElementById('research-modal-body');
    els.prModal = document.getElementById('pr-modal');
    els.closePrBtn = document.getElementById('close-pr-btn');
    els.prModalBody = document.getElementById('pr-modal-body');
    els.footerComponentCount = document.getElementById('footer-component-count');
    els.footerVerifiedCount = document.getElementById('footer-verified-count');
    els.footerCoverageMetric = document.getElementById('footer-coverage-metric');
  }

  function setupEventListeners() {
    // View mode toggle
    els.viewMode3D.addEventListener('click', () => setViewMode('3d'));
    els.viewMode2D.addEventListener('click', () => setViewMode('2d'));

    // Status overlay
    els.overlaySelect.addEventListener('change', (e) => {
      state.activeOverlay = e.target.value;
      render();
    });

    // Modals
    els.summaryBtn.addEventListener('click', openSummaryModal);
    els.closeSummaryBtn.addEventListener('click', () => els.summaryModal.classList.add('hidden'));
    els.researchBtn.addEventListener('click', openResearchModal);
    els.closeResearchBtn.addEventListener('click', () => els.researchModal.classList.add('hidden'));
    els.prPanelBtn.addEventListener('click', openPrModal);
    els.closePrBtn.addEventListener('click', () => els.prModal.classList.add('hidden'));

    // Close modal on click backdrop
    [els.summaryModal, els.researchModal, els.prModal].forEach(m => {
      m.addEventListener('click', (e) => {
        if (e.target === m) m.classList.add('hidden');
      });
    });

    // Search
    els.componentSearch.addEventListener('input', (e) => {
      state.searchQuery = e.target.value.trim().toLowerCase();
      updateSearchState();
      render();
    });
    els.clearSearchBtn.addEventListener('click', () => {
      els.componentSearch.value = '';
      state.searchQuery = '';
      updateSearchState();
      render();
    });

    // Evidence Filter Chips
    els.evidenceFilterChips.addEventListener('click', (e) => {
      const chip = e.target.closest('.chip');
      if (!chip) return;
      els.evidenceFilterChips.querySelectorAll('.chip').forEach(c => c.classList.remove('active'));
      chip.classList.add('active');
      state.activeEvidenceFilter = chip.dataset.filter;
      render();
    });

    // Layer toggle all/none
    els.btnLayersAll.addEventListener('click', () => {
      state.layers.forEach(l => state.visibleLayers.add(l.id));
      updateLayerUI();
      render();
    });
    els.btnLayersNone.addEventListener('click', () => {
      state.visibleLayers.clear();
      updateLayerUI();
      render();
    });

    // Pathway and Gap selects
    els.pathwaySelect.addEventListener('change', (e) => {
      state.activePathwayId = e.target.value;
      render();
    });
    els.gapSelect.addEventListener('change', (e) => {
      state.activeGapId = e.target.value;
      render();
    });

    // Detail Drawer close
    els.closeDetailBtn.addEventListener('click', () => {
      selectNode(null);
    });

    // Canvas Mouse Interaction (Pan, Orbit, Zoom, Select)
    const canvas = els.architectureCanvas;
    canvas.addEventListener('mousedown', (e) => {
      state.isDragging = true;
      state.dragButton = e.button;
      state.lastMouseX = e.clientX;
      state.lastMouseY = e.clientY;
    });
    window.addEventListener('mousemove', (e) => {
      if (!state.isDragging) return;
      const dx = e.clientX - state.lastMouseX;
      const dy = e.clientY - state.lastMouseY;
      state.lastMouseX = e.clientX;
      state.lastMouseY = e.clientY;

      if (state.dragButton === 0) {
        // Orbit
        state.camera.rotY += dx * 0.006;
        state.camera.rotX += dy * 0.006;
        // Clamp pitch
        state.camera.rotX = Math.max(-Math.PI / 2.2, Math.min(Math.PI / 2.2, state.camera.rotX));
      } else if (state.dragButton === 2) {
        // Pan
        state.camera.panX += dx;
        state.camera.panY += dy;
      }
      if (state.viewMode === '3d') render3D();
    });
    window.addEventListener('mouseup', () => {
      state.isDragging = false;
    });
    canvas.addEventListener('contextmenu', (e) => e.preventDefault());
    canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      const zoomFactor = e.deltaY < 0 ? 1.1 : 0.9;
      state.camera.zoom = Math.max(0.15, Math.min(3.0, state.camera.zoom * zoomFactor));
      if (state.viewMode === '3d') render3D();
    }, { passive: false });

    // Canvas click to select node
    canvas.addEventListener('click', (e) => {
      if (state.dragDistance > 5) return;
      handleCanvasClick(e);
    });

    // Reset Camera
    els.resetCameraBtn.addEventListener('click', () => {
      state.camera = {
        rotX: 0.45,
        rotY: -0.55,
        zoom: 0.55,
        panX: 0,
        panY: 50
      };
      if (state.viewMode === '3d') render3D();
    });

    // Window resize
    window.addEventListener('resize', () => {
      resizeCanvas();
      if (state.viewMode === '3d') render3D();
    });
  }

  async function loadDatasets() {
    const basePath = '.';
    try {
      const [
        layers, nodes, edges, ifaces, gaps, paths, evidence, layout, cov, research, prs
      ] = await Promise.all([
        fetchJson(`${basePath}/architecture_layers.json`),
        fetchJson(`${basePath}/architecture_nodes.json`),
        fetchJson(`${basePath}/architecture_edges.json`),
        fetchJson(`${basePath}/architecture_interfaces.json`),
        fetchJson(`${basePath}/architecture_gaps.json`),
        fetchJson(`${basePath}/execution_paths.json`),
        fetchJson(`${basePath}/component_evidence_index.json`),
        fetchJson(`${basePath}/visualization_layout.json`),
        fetchJson(`${basePath}/coverage_manifest.json`),
        fetchJson(`${basePath}/research_programs.json`),
        fetchJson(`${basePath}/pull_requests_snapshot.json`)
      ]);

      state.layers = layers.layers;
      state.nodes = nodes.nodes;
      state.edges = edges.edges;
      state.interfaces = ifaces.interfaces;
      state.gaps = gaps.gaps;
      state.executionPaths = paths.execution_paths;
      state.evidenceIndex = evidence;
      state.layout = layout;
      state.coverage = cov;
      state.researchPrograms = research;
      state.prSnapshot = prs;

      // Build Fast Lookups
      state.nodes.forEach(n => nodesById.set(n.id, n));
      state.edges.forEach(e => {
        if (!edgesBySource.has(e.source_id)) edgesBySource.set(e.source_id, []);
        edgesBySource.get(e.source_id).push(e);
        if (!edgesByTarget.has(e.target_id)) edgesByTarget.set(e.target_id, []);
        edgesByTarget.get(e.target_id).push(e);
      });
      if (state.evidenceIndex && state.evidenceIndex.components) {
        state.evidenceIndex.components.forEach(c => evidenceByComponentId.set(c.component_id, c));
      }
      if (state.prSnapshot && state.prSnapshot.pull_requests) {
        state.prSnapshot.pull_requests.forEach(pr => {
          pr.affected_subsystems.forEach(sub => {
            if (!prsBySubsystemId.has(sub)) prsBySubsystemId.set(sub, []);
            prsBySubsystemId.get(sub).push(pr);
          });
        });
      }

      // Default all layers visible
      state.layers.forEach(l => state.visibleLayers.add(l.id));

    } catch (err) {
      console.error('Failed to load architecture datasets:', err);
      showErrorMessage('Failed to load architecture datasets. Please verify HTTP server or local file access: ' + err.message);
    }
  }

  async function fetchJson(url) {
    const res = await fetch(url);
    if (!res.ok) throw new Error(`HTTP ${res.status} for ${url}`);
    return await res.json();
  }

  function showErrorMessage(msg) {
    const banner = document.createElement('div');
    banner.style.cssText = 'position:fixed;top:10px;left:50%;transform:translateX(-50%);background:#f85149;color:#fff;padding:12px 20px;border-radius:6px;z-index:9999;font-weight:600;box-shadow:0 4px 12px rgba(0,0,0,0.5);';
    banner.textContent = msg;
    document.body.appendChild(banner);
  }

  function initUI() {
    // Populate Layer Controls
    els.layersList.innerHTML = '';
    state.layers.forEach(layer => {
      const nodeCount = state.nodes.filter(n => n.layer_id === layer.id).length;
      const item = document.createElement('div');
      item.className = 'layer-item';
      item.dataset.layerId = layer.id;
      item.innerHTML = `
        <span class="layer-color-dot" style="background-color: ${layer.color_theme};"></span>
        <span class="layer-item-title">${layer.name}</span>
        <span class="layer-item-count">${nodeCount}</span>
      `;
      item.addEventListener('click', () => {
        if (state.visibleLayers.has(layer.id)) {
          state.visibleLayers.delete(layer.id);
        } else {
          state.visibleLayers.add(layer.id);
        }
        updateLayerUI();
        render();
      });
      els.layersList.appendChild(item);
    });

    // Populate Pathways Select
    state.executionPaths.forEach(p => {
      const opt = document.createElement('option');
      opt.value = p.id;
      opt.textContent = `${p.id}: ${p.name} (${p.steps.length} steps)`;
      els.pathwaySelect.appendChild(opt);
    });

    // Populate Gaps Select
    state.gaps.forEach(g => {
      const opt = document.createElement('option');
      opt.value = g.id;
      opt.textContent = `${g.id}: ${g.title} [${g.severity}]`;
      els.gapSelect.appendChild(opt);
    });

    // Footer stats
    if (els.footerComponentCount) els.footerComponentCount.textContent = state.nodes.length;
    if (state.evidenceIndex && els.footerVerifiedCount) {
      els.footerVerifiedCount.textContent = state.evidenceIndex.classification_summary.VERIFIED || 0;
    }
    if (state.coverage && els.footerCoverageMetric) {
      els.footerCoverageMetric.textContent = `${state.coverage.top_level_directory_representation_percentage}% (${state.coverage.mapped_directories.length}/${state.coverage.total_top_level_directories} dirs)`;
    }

    resizeCanvas();
    updateLayerUI();
    render2DFallback();
  }

  function updateLayerUI() {
    els.layersList.querySelectorAll('.layer-item').forEach(el => {
      const layerId = el.dataset.layerId;
      if (state.visibleLayers.has(layerId)) {
        el.classList.remove('disabled');
      } else {
        el.classList.add('disabled');
      }
    });
  }

  function updateSearchState() {
    if (!state.searchQuery) {
      els.searchResultsCount.textContent = '';
      return;
    }
    const matchCount = state.nodes.filter(n => isNodeMatch(n)).length;
    els.searchResultsCount.textContent = `${matchCount} component${matchCount === 1 ? '' : 's'} matched`;
  }

  function isNodeMatch(node) {
    if (!state.searchQuery) return true;
    const q = state.searchQuery;
    return (
      node.id.toLowerCase().includes(q) ||
      node.name.toLowerCase().includes(q) ||
      (node.description && node.description.toLowerCase().includes(q)) ||
      (node.public_interfaces && node.public_interfaces.some(i => i.toLowerCase().includes(q))) ||
      (node.source_paths && node.source_paths.some(s => s.toLowerCase().includes(q)))
    );
  }

  function isNodeVisible(node) {
    if (!state.visibleLayers.has(node.layer_id)) return false;

    // Evidence filter
    if (state.activeEvidenceFilter !== 'ALL') {
      const ev = evidenceByComponentId.get(node.id);
      const classification = ev ? ev.relationship_classification : 'NOT_MAPPED';
      if (classification !== state.activeEvidenceFilter) return false;
    }

    // Pathway filter
    if (state.activePathwayId !== 'none') {
      const path = state.executionPaths.find(p => p.id === state.activePathwayId);
      if (path) {
        const inPath = path.steps.some(s => s.node_id === node.id);
        if (!inPath) return false;
      }
    }

    // Gap filter
    if (state.activeGapId !== 'none') {
      const gap = state.gaps.find(g => g.id === state.activeGapId);
      if (gap) {
        const inGap = (gap.affected_nodes && gap.affected_nodes.includes(node.id));
        if (!inGap) return false;
      }
    }

    return true;
  }

  function setViewMode(mode) {
    state.viewMode = mode;
    if (mode === '3d') {
      els.viewMode3D.classList.add('active');
      els.viewMode2D.classList.remove('active');
      els.canvasContainer.classList.remove('hidden');
      els.fallback2DContainer.classList.add('hidden');
      render3D();
    } else {
      els.viewMode3D.classList.remove('active');
      els.viewMode2D.classList.add('active');
      els.canvasContainer.classList.add('hidden');
      els.fallback2DContainer.classList.remove('hidden');
      render2DFallback();
    }
  }

  function render() {
    if (state.viewMode === '3d') {
      render3D();
    } else {
      render2DFallback();
    }
  }

  // ==========================================
  // Status Overlay Computation
  // ==========================================
  function getNodeOverlayBadge(node) {
    const ev = evidenceByComponentId.get(node.id);
    const classification = ev ? ev.relationship_classification : 'NOT_MAPPED';

    switch (state.activeOverlay) {
      case 'evidence':
        return {
          label: classification,
          cls: classification === 'VERIFIED' ? 'tag-verified' :
               classification === 'STATICALLY_SUPPORTED' ? 'tag-static' :
               classification === 'UNVERIFIED' ? 'tag-unverified' : 'tag-not-mapped',
          color: classification === 'VERIFIED' ? '#238636' :
                 classification === 'STATICALLY_SUPPORTED' ? '#1f6feb' :
                 classification === 'UNVERIFIED' ? '#d29922' : '#8b949e'
        };

      case 'maturity':
        const prod = node.maturity ? node.maturity.production_status : 'UNKNOWN';
        const isExp = node.maturity && node.maturity.execution === 'EXPERIMENTAL';
        const isProp = prod === 'ARCHITECTURAL_PROPOSAL' || (node.maturity && !node.maturity.source_presence);
        const label = isProp ? 'PROPOSED' : (isExp ? 'EXPERIMENTAL' : (prod === 'PRODUCTION_CORE' ? 'IMPLEMENTED' : 'UNKNOWN'));
        return {
          label: label,
          cls: label === 'IMPLEMENTED' ? 'tag-verified' : label === 'EXPERIMENTAL' ? 'tag-unverified' : 'tag-not-mapped',
          color: label === 'IMPLEMENTED' ? '#238636' : label === 'EXPERIMENTAL' ? '#d29922' : '#a371f7'
        };

      case 'qualification':
        const qual = node.maturity ? node.maturity.qualification : 'UNVERIFIED';
        const qualLabel = qual === 'QUALIFIED' ? 'QUALIFIED' :
                          qual === 'QUALIFIED_WITH_LIMITATIONS' ? 'QUALIFIED (LIMITATIONS)' :
                          qual === 'BLOCKED' ? 'BLOCKED' : 'NOT EVALUATED';
        return {
          label: qualLabel,
          cls: qual.startsWith('QUALIFIED') ? 'tag-verified' : qual === 'BLOCKED' ? 'tag-blocked' : 'tag-unverified',
          color: qual.startsWith('QUALIFIED') ? '#238636' : qual === 'BLOCKED' ? '#f85149' : '#d29922'
        };

      case 'activity':
        const prs = prsBySubsystemId.get(node.id) || [];
        const hasOpen = prs.some(p => p.state === 'OPEN');
        const hasMerged = prs.some(p => p.state === 'MERGED');
        const actLabel = hasOpen ? 'ACTIVE PR' : (hasMerged ? 'MERGED MILESTONE' : 'NO RECORDED ACTIVITY');
        return {
          label: actLabel,
          cls: hasOpen ? 'tag-static' : (hasMerged ? 'tag-verified' : 'tag-not-mapped'),
          color: hasOpen ? '#58a6ff' : (hasMerged ? '#238636' : '#6b768a')
        };

      default:
        return { label: classification, cls: 'tag-static', color: '#1f6feb' };
    }
  }

  // ==========================================
  // Accessible 2D Fallback Rendering
  // ==========================================
  function render2DFallback() {
    els.fallbackGrid.innerHTML = '';

    state.layers.forEach(layer => {
      if (!state.visibleLayers.has(layer.id)) return;

      const layerNodes = state.nodes.filter(n => n.layer_id === layer.id && isNodeVisible(n));
      if (layerNodes.length === 0) return;

      const group = document.createElement('section');
      group.className = 'layer-card-group';
      group.setAttribute('aria-label', `Layer ${layer.index}: ${layer.name}`);

      const header = document.createElement('div');
      header.className = 'layer-card-header';
      header.innerHTML = `
        <span class="layer-badge" style="background-color: ${layer.color_theme};">L${layer.index}</span>
        <h3 class="layer-name-title">${layer.name}</h3>
        <span class="layer-item-count">(${layerNodes.length} component${layerNodes.length === 1 ? '' : 's'})</span>
      `;
      group.appendChild(header);

      const grid = document.createElement('div');
      grid.className = 'layer-nodes-grid';

      layerNodes.forEach(node => {
        const card = document.createElement('article');
        card.className = 'node-card' + (node.id === state.selectedNodeId ? ' selected' : '');
        card.setAttribute('tabindex', '0');
        card.setAttribute('role', 'button');
        card.setAttribute('aria-label', `Select ${node.name}`);

        const overlayBadge = getNodeOverlayBadge(node);
        const ev = evidenceByComponentId.get(node.id);
        const classification = ev ? ev.relationship_classification : 'NOT_MAPPED';

        card.innerHTML = `
          <div class="node-card-top">
            <h4 class="node-card-name">${node.name}</h4>
            <span class="status-tag ${overlayBadge.cls}">${overlayBadge.label}</span>
          </div>
          <div class="node-card-id">${node.id}</div>
          <p class="node-card-desc">${node.description || 'No description provided.'}</p>
          <div class="node-card-badges">
            <span class="status-tag ${classification === 'VERIFIED' ? 'tag-verified' : 'tag-static'}">${classification}</span>
            <span class="status-tag tag-not-mapped">${node.category}</span>
          </div>
        `;

        card.addEventListener('click', () => selectNode(node.id));
        card.addEventListener('keydown', (e) => {
          if (e.key === 'Enter' || e.key === ' ') {
            e.preventDefault();
            selectNode(node.id);
          }
        });

        grid.appendChild(card);
      });

      group.appendChild(grid);
      els.fallbackGrid.appendChild(group);
    });

    if (els.fallbackGrid.children.length === 0) {
      els.fallbackGrid.innerHTML = `
        <div class="detail-empty-state">
          <p>No components match the currently active filters and layer selections.</p>
        </div>
      `;
    }
  }

  // ==========================================
  // Interactive 3D Canvas View (WebGL / Isometric Canvas)
  // ==========================================
  let projectedNodes = [];

  function resizeCanvas() {
    const canvas = els.architectureCanvas;
    const rect = els.canvasContainer.getBoundingClientRect();
    if (rect.width === 0 || rect.height === 0) return;
    canvas.width = rect.width * window.devicePixelRatio;
    canvas.height = rect.height * window.devicePixelRatio;
  }

  function render3D() {
    const canvas = els.architectureCanvas;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    ctx.save();
    ctx.scale(window.devicePixelRatio, window.devicePixelRatio);
    const width = canvas.width / window.devicePixelRatio;
    const height = canvas.height / window.devicePixelRatio;

    // Clear background
    ctx.fillStyle = '#12141a';
    ctx.fillRect(0, 0, width, height);

    // Compute Camera Transform
    const cx = width / 2 + state.camera.panX;
    const cy = height / 2 + state.camera.panY;
    const cosX = Math.cos(state.camera.rotX);
    const sinX = Math.sin(state.camera.rotX);
    const cosY = Math.cos(state.camera.rotY);
    const sinY = Math.sin(state.camera.rotY);
    const zoom = state.camera.zoom;

    // 3D Point Projection Helper
    function project(x, y, z) {
      // Rotate around Y
      const x1 = x * cosY + z * sinY;
      const z1 = -x * sinY + z * cosY;
      // Rotate around X
      const y2 = y * cosX - z1 * sinX;
      const z2 = y * sinX + z1 * cosX;

      // Invert Y coordinate so higher elevation is up
      const sx = cx + x1 * zoom;
      const sy = cy - y2 * zoom;
      return { sx, sy, depth: z2 };
    }

    // Render Layer Elevation Planes
    state.layers.forEach(layer => {
      if (!state.visibleLayers.has(layer.id)) return;
      const y = layer.y_level - 300; // Center Y
      const planeSize = 450;
      const p1 = project(-planeSize, y, -planeSize);
      const p2 = project(planeSize, y, -planeSize);
      const p3 = project(planeSize, y, planeSize);
      const p4 = project(-planeSize, y, planeSize);

      ctx.beginPath();
      ctx.moveTo(p1.sx, p1.sy);
      ctx.lineTo(p2.sx, p2.sy);
      ctx.lineTo(p3.sx, p3.sy);
      ctx.lineTo(p4.sx, p4.sy);
      ctx.closePath();
      ctx.fillStyle = 'rgba(31, 36, 48, 0.25)';
      ctx.fill();
      ctx.strokeStyle = layer.color_theme + '33';
      ctx.lineWidth = 1;
      ctx.stroke();

      // Layer Label
      const pLabel = project(-planeSize + 20, y, -planeSize + 20);
      ctx.fillStyle = layer.color_theme;
      ctx.font = '10px -apple-system, sans-serif';
      ctx.fillText(`L${layer.index}: ${layer.name}`, pLabel.sx, pLabel.sy);
    });

    // Compute Projected Node Positions from visualization_layout.json
    projectedNodes = [];
    const layoutPositions = state.layout ? state.layout.node_positions : {};

    state.nodes.forEach(node => {
      if (!isNodeVisible(node)) return;

      const pos = layoutPositions[node.id] || { x: 0, y: 0, z: 0 };
      const y = pos.y - 300; // Center elevation
      const p = project(pos.x, y, pos.z);

      const overlayBadge = getNodeOverlayBadge(node);
      const isSelected = node.id === state.selectedNodeId;
      const isMatch = isNodeMatch(node);

      projectedNodes.push({
        node,
        sx: p.sx,
        sy: p.sy,
        depth: p.depth,
        color: overlayBadge.color,
        isSelected,
        isMatch
      });
    });

    // Sort by depth for correct painter's algorithm
    projectedNodes.sort((a, b) => a.depth - b.depth);

    // Render Edges
    const visibleNodeIds = new Set(projectedNodes.map(pn => pn.node.id));
    const projMap = new Map(projectedNodes.map(pn => [pn.node.id, pn]));

    state.edges.forEach(edge => {
      if (!visibleNodeIds.has(edge.source_id) || !visibleNodeIds.has(edge.target_id)) return;
      const s = projMap.get(edge.source_id);
      const t = projMap.get(edge.target_id);
      if (!s || !t) return;

      const isRelated = state.selectedNodeId &&
        (edge.source_id === state.selectedNodeId || edge.target_id === state.selectedNodeId);

      ctx.beginPath();
      ctx.moveTo(s.sx, s.sy);
      ctx.lineTo(t.sx, t.sy);
      ctx.strokeStyle = isRelated ? '#58a6ff' : 'rgba(110, 118, 129, 0.25)';
      ctx.lineWidth = isRelated ? 2 : 1;
      ctx.stroke();
    });

    // Render Nodes (Disks + Labels)
    projectedNodes.forEach(pn => {
      const radius = pn.isSelected ? 8 : (pn.isMatch ? 6 : 4);

      // Node Disk
      ctx.beginPath();
      ctx.arc(pn.sx, pn.sy, radius, 0, Math.PI * 2);
      ctx.fillStyle = pn.color;
      ctx.fill();

      if (pn.isSelected) {
        ctx.strokeStyle = '#ffffff';
        ctx.lineWidth = 2.5;
        ctx.stroke();
      } else {
        ctx.strokeStyle = 'rgba(0,0,0,0.5)';
        ctx.lineWidth = 1;
        ctx.stroke();
      }

      // Label
      if (pn.isSelected || pn.isMatch || zoom > 0.8) {
        ctx.font = pn.isSelected ? 'bold 11px -apple-system, sans-serif' : '9px -apple-system, sans-serif';
        ctx.fillStyle = pn.isSelected ? '#ffffff' : (pn.isMatch ? '#e6edf3' : '#8b949e');
        ctx.fillText(pn.node.name, pn.sx + radius + 4, pn.sy + 3);
      }
    });

    ctx.restore();
  }

  function handleCanvasClick(e) {
    const canvas = els.architectureCanvas;
    const rect = canvas.getBoundingClientRect();
    const clickX = e.clientX - rect.left;
    const clickY = e.clientY - rect.top;

    let nearest = null;
    let minDist = 18; // Hit radius in CSS pixels

    projectedNodes.forEach(pn => {
      const dist = Math.hypot(clickX - pn.sx, clickY - pn.sy);
      if (dist < minDist) {
        minDist = dist;
        nearest = pn.node;
      }
    });

    selectNode(nearest ? nearest.id : null);
  }

  // ==========================================
  // Component Selection and Detail Inspection
  // ==========================================
  function selectNode(nodeId) {
    state.selectedNodeId = nodeId;

    if (!nodeId) {
      els.detailTitle.textContent = 'Component Details';
      els.detailEmpty.classList.remove('hidden');
      els.detailContent.classList.add('hidden');
      els.detailContent.innerHTML = '';
      render();
      return;
    }

    const node = nodesById.get(nodeId);
    if (!node) return;

    els.detailTitle.textContent = node.name;
    els.detailEmpty.classList.add('hidden');
    els.detailContent.classList.remove('hidden');

    const ev = evidenceByComponentId.get(node.id);
    const classification = ev ? ev.relationship_classification : 'NOT_MAPPED';
    const overlayBadge = getNodeOverlayBadge(node);
    const layer = state.layers.find(l => l.id === node.layer_id);
    const prs = prsBySubsystemId.get(node.id) || [];

    // Upstream & Downstream Dependencies
    const upEdges = edgesByTarget.get(node.id) || [];
    const downEdges = edgesBySource.get(node.id) || [];

    els.detailContent.innerHTML = `
      <!-- Status & Identity -->
      <div class="detail-section">
        <span class="detail-section-title">Identity &amp; Governance</span>
        <div class="detail-prop-row">
          <span class="detail-prop-key">ID:</span>
          <span class="detail-prop-val">${node.id}</span>
        </div>
        <div class="detail-prop-row">
          <span class="detail-prop-key">Layer:</span>
          <span class="detail-prop-val">${layer ? layer.name : node.layer_id}</span>
        </div>
        <div class="detail-prop-row">
          <span class="detail-prop-key">Category:</span>
          <span class="detail-prop-val">${node.category}</span>
        </div>
        <div class="detail-prop-row">
          <span class="detail-prop-key">Evidence Status:</span>
          <span class="status-tag ${classification === 'VERIFIED' ? 'tag-verified' : 'tag-static'}">${classification}</span>
        </div>
        <div class="detail-prop-row">
          <span class="detail-prop-key">Active Overlay (${state.activeOverlay}):</span>
          <span class="status-tag ${overlayBadge.cls}">${overlayBadge.label}</span>
        </div>
      </div>

      <!-- Description -->
      <div class="detail-section">
        <span class="detail-section-title">Description</span>
        <p style="font-size:0.8rem; color:var(--text-secondary);">${node.description || 'No description provided.'}</p>
      </div>

      <!-- Implementation Maturity -->
      <div class="detail-section">
        <span class="detail-section-title">Implementation Maturity</span>
        <div class="detail-prop-row">
          <span class="detail-prop-key">Source Presence:</span>
          <span class="detail-prop-val">${node.maturity && node.maturity.source_presence ? 'YES' : 'NO'}</span>
        </div>
        <div class="detail-prop-row">
          <span class="detail-prop-key">Compilation:</span>
          <span class="detail-prop-val">${node.maturity && node.maturity.compilation ? 'YES' : 'NO'}</span>
        </div>
        <div class="detail-prop-row">
          <span class="detail-prop-key">Execution Profile:</span>
          <span class="detail-prop-val">${node.maturity ? node.maturity.execution : 'N/A'}</span>
        </div>
        <div class="detail-prop-row">
          <span class="detail-prop-key">Test Coverage:</span>
          <span class="detail-prop-val">${node.maturity ? node.maturity.test_coverage : 'NONE'}</span>
        </div>
        <div class="detail-prop-row">
          <span class="detail-prop-key">Qualification Level:</span>
          <span class="detail-prop-val">${node.maturity ? node.maturity.qualification : 'UNVERIFIED'}</span>
        </div>
        <div class="detail-prop-row">
          <span class="detail-prop-key">Production Status:</span>
          <span class="detail-prop-val">${node.maturity ? node.maturity.production_status : 'N/A'}</span>
        </div>
      </div>

      <!-- Evidence Notes & Continuity Receipts -->
      <div class="detail-section">
        <span class="detail-section-title">Evidence &amp; Continuity Receipts</span>
        <p style="font-size:0.75rem; color:var(--text-muted); margin-bottom:4px;">
          ${ev && ev.evidence_notes ? ev.evidence_notes : 'No evidence notes recorded.'}
        </p>
        ${ev && ev.receipt_files && ev.receipt_files.length > 0 ? `
          <div class="file-link-list">
            ${ev.receipt_files.map(r => `<span class="file-link-item">${r}</span>`).join('')}
          </div>
        ` : '<span style="font-size:0.75rem; color:var(--text-muted);">No formal receipts bound (unverified for independent qualification).</span>'}
      </div>

      <!-- CMake Test Targets -->
      <div class="detail-section">
        <span class="detail-section-title">CMake Test Targets</span>
        ${ev && ev.cmake_test_targets && ev.cmake_test_targets.length > 0 ? `
          <div class="file-link-list">
            ${ev.cmake_test_targets.map(t => `<span class="file-link-item">${t}</span>`).join('')}
          </div>
        ` : '<span style="font-size:0.75rem; color:var(--text-muted);">No standalone CMake test targets.</span>'}
      </div>

      <!-- Source Files -->
      <div class="detail-section">
        <span class="detail-section-title">Tracked Source Files</span>
        ${node.source_paths && node.source_paths.length > 0 ? `
          <div class="file-link-list">
            ${node.source_paths.map(s => `<span class="file-link-item">${s}</span>`).join('')}
          </div>
        ` : '<span style="font-size:0.75rem; color:var(--text-muted);">No direct source files.</span>'}
      </div>

      <!-- Architectural Relationships -->
      <div class="detail-section">
        <span class="detail-section-title">Dependencies &amp; Relationships</span>
        <div class="detail-prop-row">
          <span class="detail-prop-key">Upstream Providers (${downEdges.length}):</span>
        </div>
        ${downEdges.length > 0 ? `
          <div class="file-link-list">
            ${downEdges.map(e => `<span class="file-link-item">&rarr; ${e.target_id} [${e.dependency_type}]</span>`).join('')}
          </div>
        ` : '<span style="font-size:0.75rem; color:var(--text-muted);">No outbound dependencies.</span>'}
        <div class="detail-prop-row" style="margin-top:6px;">
          <span class="detail-prop-key">Downstream Dependents (${upEdges.length}):</span>
        </div>
        ${upEdges.length > 0 ? `
          <div class="file-link-list">
            ${upEdges.map(e => `<span class="file-link-item">&larr; ${e.source_id} [${e.dependency_type}]</span>`).join('')}
          </div>
        ` : '<span style="font-size:0.75rem; color:var(--text-muted);">No inbound dependents.</span>'}
      </div>

      <!-- Pull Request Activity -->
      <div class="detail-section">
        <span class="detail-section-title">Associated Pull Requests (${prs.length})</span>
        ${prs.length > 0 ? `
          <div class="file-link-list">
            ${prs.map(pr => `
              <div class="file-link-item" style="display:flex; justify-content:space-between; align-items:center;">
                <span>#${pr.pr_number} ${pr.title.slice(0, 32)}...</span>
                <span class="status-tag ${pr.state === 'MERGED' ? 'tag-verified' : 'tag-static'}">${pr.state}</span>
              </div>
            `).join('')}
          </div>
        ` : '<span style="font-size:0.75rem; color:var(--text-muted);">No active or recent PRs mapped to this component.</span>'}
      </div>
    `;

    render();
  }

  // ==========================================
  // Summary Metrics Modal
  // ==========================================
  function openSummaryModal() {
    const evSummary = state.evidenceIndex ? state.evidenceIndex.classification_summary : {};
    const cov = state.coverage || {};
    const prSummary = state.prSnapshot ? state.prSnapshot.pr_inventory_summary : {};

    // Compute Qualification Distribution
    let qualCounts = { QUALIFIED: 0, QUALIFIED_WITH_LIMITATIONS: 0, UNVERIFIED: 0, NOT_APPLICABLE: 0 };
    state.nodes.forEach(n => {
      const q = n.maturity ? n.maturity.qualification : 'UNVERIFIED';
      if (qualCounts[q] !== undefined) qualCounts[q]++;
      else qualCounts['UNVERIFIED']++;
    });

    els.summaryModalBody.innerHTML = `
      <!-- Notice on Non-Universal Metrics -->
      <div style="background:rgba(210,153,34,0.15); border:1px solid rgba(210,153,34,0.4); padding:10px 14px; border-radius:6px; color:#f0883e; font-size:0.8rem;">
        <strong>Methodological Constraint:</strong> All metrics derive strictly from inspected repository evidence and research receipts. The T81 Foundation does not publish an aggregate "overall system completion %" because implementation presence, test execution, and independent mathematical qualification represent fundamentally distinct dimensions.
      </div>

      <!-- Section: Evidence Classifications -->
      <div>
        <h3 style="font-size:0.95rem; margin-bottom:8px;">Component Evidence Classifications (${state.nodes.length} Components)</h3>
        <table class="data-table">
          <thead>
            <tr>
              <th>Classification</th>
              <th>Count</th>
              <th>Percentage</th>
              <th>Prerequisites / Semantic Meaning</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <td><span class="status-tag tag-verified">VERIFIED</span></td>
              <td><strong>${evSummary.VERIFIED || 0}</strong></td>
              <td>${((evSummary.VERIFIED || 0) / state.nodes.length * 100).toFixed(1)}%</td>
              <td>Formal continuity receipt + dedicated tests + QUALIFIED maturity (SCG-07 & CPA-004)</td>
            </tr>
            <tr>
              <td><span class="status-tag tag-static">STATICALLY_SUPPORTED</span></td>
              <td><strong>${evSummary.STATICALLY_SUPPORTED || 0}</strong></td>
              <td>${((evSummary.STATICALLY_SUPPORTED || 0) / state.nodes.length * 100).toFixed(1)}%</td>
              <td>Tracked C++ source code + compiled tests/targets; no independent continuity qualification receipt</td>
            </tr>
            <tr>
              <td><span class="status-tag tag-unverified">UNVERIFIED</span></td>
              <td><strong>${evSummary.UNVERIFIED || 0}</strong></td>
              <td>${((evSummary.UNVERIFIED || 0) / state.nodes.length * 100).toFixed(1)}%</td>
              <td>Draft or experimental code lacking dedicated test coverage</td>
            </tr>
            <tr>
              <td><span class="status-tag tag-not-mapped">NOT_MAPPED</span></td>
              <td><strong>${evSummary.NOT_MAPPED || 0}</strong></td>
              <td>${((evSummary.NOT_MAPPED || 0) / state.nodes.length * 100).toFixed(1)}%</td>
              <td>Specification or target architecture lacking internal source representation (DAIOS RFC-00D2)</td>
            </tr>
          </tbody>
        </table>
      </div>

      <!-- Section: Research Qualification Distribution -->
      <div>
        <h3 style="font-size:0.95rem; margin-bottom:8px;">Research Qualification Distribution</h3>
        <table class="data-table">
          <thead>
            <tr>
              <th>Qualification Level</th>
              <th>Count</th>
              <th>Scope</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <td><span class="status-tag tag-verified">QUALIFIED</span></td>
              <td><strong>${qualCounts.QUALIFIED}</strong></td>
              <td>Rigorous mathematical oracle verification without documented anomalies</td>
            </tr>
            <tr>
              <td><span class="status-tag tag-verified">QUALIFIED_WITH_LIMITATIONS</span></td>
              <td><strong>${qualCounts.QUALIFIED_WITH_LIMITATIONS}</strong></td>
              <td>SCG-07 Phase 6 arithmetic (F3 modular invariant, F7 bounded binary64); CPA-004 freestanding runtime</td>
            </tr>
            <tr>
              <td><span class="status-tag tag-unverified">UNVERIFIED</span></td>
              <td><strong>${qualCounts.UNVERIFIED}</strong></td>
              <td>Uncertified for independent qualification (compilers, VM interpreter, CanonFS, CLI, Axion)</td>
            </tr>
          </tbody>
        </table>
      </div>

      <!-- Section: Repository Coverage -->
      <div>
        <h3 style="font-size:0.95rem; margin-bottom:8px;">Repository Directory Coverage</h3>
        <p style="font-size:0.8rem; color:var(--text-secondary); margin-bottom:6px;">
          Top-Level Directory Representation: <strong>${cov.top_level_directory_representation_percentage}%</strong> (${cov.mapped_directories ? cov.mapped_directories.length : 0} of ${cov.total_top_level_directories || 35} directories mapped to architectural components).
        </p>
        <p style="font-size:0.75rem; color:var(--text-muted);">
          Unmapped directories represent tooling, CI configs, benchmarks, and external fixtures not part of runtime subsystems:
          <br><code>${cov.unmapped_directories ? cov.unmapped_directories.join(', ') : ''}</code>
        </p>
      </div>

      <!-- Section: Key Blockers & Investigations -->
      <div>
        <h3 style="font-size:0.95rem; margin-bottom:8px;">Key Engineering Blockers &amp; Status</h3>
        <ul style="font-size:0.8rem; color:var(--text-secondary); padding-left:20px; display:flex; flex-direction:column; gap:6px;">
          <li><strong>TernaryOS QEMU Native Verification:</strong> Fail-closed verifier active (PR #756). Phase 3 investigation (PR #757) confirms firmware diagnostic barriers; native 5-check verification remains NOT QUALIFIED.</li>
          <li><strong>SCG-08 Gate 1 &amp; 2:</strong> Phase 0 research charter integrated (PR #753). Signed-domain amendment pending recorded maintainer decision; implementation and execution authorization strictly withheld.</li>
          <li><strong>H04-R03 Codec Benchmark:</strong> Candidate A correctness and weak-reference retention verified (PR #727); Phase C performance measurement remains frozen.</li>
        </ul>
      </div>
    `;

    els.summaryModal.classList.remove('hidden');
  }

  // ==========================================
  // Research Programs Modal
  // ==========================================
  function openResearchModal() {
    if (!state.researchPrograms) return;

    const progs = state.researchPrograms.programs || [];
    const hyps = state.researchPrograms.hypothesis_mappings || [];

    els.researchModalBody.innerHTML = `
      <div style="margin-bottom:12px;">
        <p style="font-size:0.85rem; color:var(--text-secondary);">
          Evidence-linked status of active research initiatives across the T81 Foundation platform. All entries cite authoritative receipts and pull requests.
        </p>
      </div>

      <!-- Programs Accordion -->
      <div style="display:flex; flex-direction:column; gap:14px;">
        ${progs.map(prog => `
          <div style="background:var(--bg-card); border:1px solid var(--border-color); border-radius:6px; padding:12px;">
            <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:6px;">
              <h3 style="font-size:0.95rem; color:var(--accent-hover);">${prog.name}</h3>
              <span class="status-tag ${prog.status.includes('ACTIVE') ? 'tag-static' : 'tag-verified'}">${prog.status}</span>
            </div>
            <p style="font-size:0.8rem; color:var(--text-secondary); margin-bottom:8px;">${prog.summary}</p>

            <!-- Phases Table -->
            <table class="data-table">
              <thead>
                <tr>
                  <th style="width:15%;">Phase</th>
                  <th style="width:30%;">Title</th>
                  <th style="width:25%;">Disposition</th>
                  <th style="width:30%;">Receipt / Details</th>
                </tr>
              </thead>
              <tbody>
                ${prog.phases.map(ph => `
                  <tr>
                    <td><code>${ph.phase_id}</code></td>
                    <td>${ph.title}</td>
                    <td><span class="status-tag ${ph.disposition.includes('QUALIFIED') || ph.disposition.includes('VERIFIED') ? 'tag-verified' : (ph.disposition.includes('WITHHELD') || ph.disposition.includes('REPAIR_NOT_AUTHORIZED') ? 'tag-blocked' : 'tag-static')}">${ph.disposition}</span></td>
                    <td style="font-size:0.75rem;">
                      <div><code>${ph.receipt}</code></div>
                      <div style="color:var(--text-muted);">${ph.details}</div>
                    </td>
                  </tr>
                `).join('')}
              </tbody>
            </table>
          </div>
        `).join('')}
      </div>

      <!-- Hypothesis Register H01-H08 Mapping -->
      <div style="margin-top:20px;">
        <h3 style="font-size:1rem; margin-bottom:10px;">Empirical Hypothesis Register (H01–H08) Current Dispositions</h3>
        <table class="data-table">
          <thead>
            <tr>
              <th style="width:10%;">ID</th>
              <th style="width:30%;">Hypothesis Claim</th>
              <th style="width:25%;">Current Disposition</th>
              <th style="width:35%;">Governance Limitation</th>
            </tr>
          </thead>
          <tbody>
            ${hyps.map(h => `
              <tr>
                <td><strong>${h.hypothesis_id}</strong></td>
                <td>${h.title}</td>
                <td><span class="status-tag ${h.current_disposition.includes('NEGATIVE') ? 'tag-blocked' : (h.current_disposition.includes('THEORETICALLY') ? 'tag-static' : 'tag-unverified')}">${h.current_disposition}</span></td>
                <td style="font-size:0.75rem; color:var(--text-secondary);">${h.governance_note}</td>
              </tr>
            `).join('')}
          </tbody>
        </table>
      </div>
    `;

    els.researchModal.classList.remove('hidden');
  }

  // ==========================================
  // Pull Request Activity Modal
  // ==========================================
  function openPrModal() {
    if (!state.prSnapshot) return;

    const prs = state.prSnapshot.pull_requests || [];
    const summary = state.prSnapshot.pr_inventory_summary || {};

    els.prModalBody.innerHTML = `
      <div style="background:var(--bg-tertiary); border:1px solid var(--border-color); padding:10px 14px; border-radius:6px; font-size:0.8rem; color:var(--text-secondary); margin-bottom:12px;">
        <div><strong>Snapshot Timestamp:</strong> <code>${state.prSnapshot.snapshot_timestamp}</code> (Baseline SHA: <code>${state.prSnapshot.authoritative_baseline_commit.slice(0, 10)}</code>)</div>
        <div><strong>Inventory:</strong> ${summary.total_open_prs_observed} open PRs observed on remote repository; ${summary.cataloged_prs} cataloged and mapped to subsystems.</div>
        <div style="font-size:0.7rem; color:var(--text-muted); margin-top:4px;">${state.prSnapshot.refresh_instructions}</div>
      </div>

      <table class="data-table">
        <thead>
          <tr>
            <th style="width:8%;">PR #</th>
            <th style="width:28%;">Title</th>
            <th style="width:10%;">State</th>
            <th style="width:24%;">Affected Subsystems</th>
            <th style="width:30%;">Disposition / Checks</th>
          </tr>
        </thead>
        <tbody>
          ${prs.map(pr => `
            <tr>
              <td><strong>#${pr.pr_number}</strong></td>
              <td>${pr.title}</td>
              <td><span class="status-tag ${pr.state === 'MERGED' ? 'tag-verified' : 'tag-static'}">${pr.state}</span></td>
              <td style="font-size:0.75rem;">
                ${pr.affected_subsystems.map(sub => `<div><code>${sub}</code></div>`).join('')}
              </td>
              <td style="font-size:0.75rem;">
                <div><span class="status-tag ${pr.checks_status.includes('PASSED') ? 'tag-verified' : 'tag-blocked'}">${pr.checks_status}</span></div>
                <div style="color:var(--text-secondary); margin-top:2px;">${pr.review_disposition}</div>
              </td>
            </tr>
          `).join('')}
        </tbody>
      </table>
    `;

    els.prModal.classList.remove('hidden');
  }

})();
