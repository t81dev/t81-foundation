// tools/playground/web/playground.js
// T81Lang Playground — Wasm glue + editor + output logic

// ── Sample programs ──────────────────────────────────────────────────────────

const SAMPLES = {
  "hello_world": {
    label: "Hello, World!",
    code: `// T81Lang — Hello, World!
fn main() -> i32 {
  print("Hello, World!");
  return 0;
}
`
  },

  "bigint_arithmetic": {
    label: "BigInt Arithmetic",
    code: `// T81BigInt — balanced ternary big integers
fn main() -> i32 {
  let a: T81BigInt = 42t81;
  let b: T81BigInt = -17t81;
  let sum: T81BigInt = a + b;
  let product: T81BigInt = a * 2t81;
  print(sum);
  print(product);
  return 0;
}
`
  },

  "if_else": {
    label: "If / Else",
    code: `// Conditional branching
fn classify(x: i32) -> i32 {
  if (x > 0) {
    return 1;
  } else if (x < 0) {
    return -1;
  } else {
    return 0;
  }
}

fn main() -> i32 {
  print(classify(42));
  print(classify(-7));
  print(classify(0));
  return 0;
}
`
  },

  "match_option": {
    label: "Match / Option",
    code: `// Pattern matching with Option and Result
fn describe(v: Option[i32]) -> i32 {
  return match (v) {
    Some(x) => x * 2;
    None => -1;
  };
}

fn main() -> i32 {
  let a: Option[i32] = Some(21);
  let b: Option[i32] = None;
  print(describe(a));
  print(describe(b));
  return 0;
}
`
  },

  "symbol_equality": {
    label: "Symbol Equality",
    code: `// Symbols — interned identifiers for ternary pattern dispatch
fn main() -> i32 {
  let a: Symbol = :alpha;
  let b: Symbol = :alpha;
  let c: Symbol = :beta;
  var out: i32 = 0;
  if (a == b) { out = 81; } else { out = 0; }
  print(out);
  if (a == c) { out = 1;  } else { out = 0; }
  print(out);
  return 0;
}
`
  },

  "tensor_vec_add": {
    label: "Tensor Vec-Add",
    code: `// Tensor operations via std.tensor
fn main() -> i32 {
  let a: Tensor = std.tensor.from_list([1, 2, 3]);
  let b: Tensor = std.tensor.from_list([4, 5, 6]);
  let c: Tensor = std.tensor.vec_add(a, b);
  let _ = c;
  print("vec_add complete");
  return 0;
}
`
  },

  "agent_behavior": {
    label: "Agent / Behavior (RFC-0015)",
    code: `// RFC-0015 — Agent with behavior dispatch
agent Calculator {
  behavior add(a: i32, b: i32) -> i32 {
    return a + b;
  }
}

fn main() -> i32 {
  let result: i32 = Calculator.add(38, 4);
  print(result);
  return 0;
}
`
  },

  "bounded_loop": {
    label: "Bounded Loop",
    code: `// @bounded loops — required by T81Lang semantics for termination proofs
fn sum_to(n: i32) -> i32 {
  var total: i32 = 0;
  var i: i32 = 0;
  @bounded(200)
  loop {
    if (i >= n) { break; }
    total = total + i;
    i = i + 1;
  }
  return total;
}

fn main() -> i32 {
  print(sum_to(10));
  return 0;
}
`
  }
};

// ── State ────────────────────────────────────────────────────────────────────

let wasmModule = null;   // T81VM Emscripten module instance
let editor = null;       // CodeMirror EditorView
let activeTab = "output";

// ── Wasm loader ──────────────────────────────────────────────────────────────

async function loadWasm() {
  try {
    // T81VM module is loaded via <script> tag; T81VM() is the factory.
    wasmModule = await T81VM();
    hideLoading();
    setStatus("idle", "Ready");
  } catch (e) {
    showLoadError(e.message || String(e));
  }
}

function hideLoading() {
  const el = document.getElementById("loading-overlay");
  if (el) el.style.display = "none";
}

function showLoadError(msg) {
  const el = document.getElementById("loading-overlay");
  if (el) {
    el.innerHTML = `
      <div style="color:#ff5555;font-family:monospace;text-align:center;max-width:480px;padding:24px">
        <div style="font-size:18px;margin-bottom:12px">Failed to load T81VM</div>
        <div style="font-size:13px;color:#888">${escHtml(msg)}</div>
        <div style="margin-top:16px;font-size:12px;color:#555">
          Build t81vm.js/.wasm and place them next to index.html, then reload.
        </div>
      </div>`;
  }
}

// ── Editor setup (CodeMirror 6) ──────────────────────────────────────────────

function initEditor(initialCode) {
  const { EditorView, basicSetup } = CM;
  const { EditorState } = CM;
  const { oneDark } = CM;

  editor = new EditorView({
    doc: initialCode,
    extensions: [
      basicSetup,
      oneDark,
      EditorView.theme({
        "&": { backgroundColor: "var(--bg)", height: "100%" },
        ".cm-scroller": { overflow: "auto" },
        ".cm-content": { caretColor: "var(--accent)" },
      }),
      EditorView.lineWrapping,
    ],
    parent: document.getElementById("editor-container"),
  });
}

function getEditorCode() {
  return editor ? editor.state.doc.toString() : "";
}

function setEditorCode(code) {
  if (!editor) return;
  editor.dispatch({
    changes: { from: 0, to: editor.state.doc.length, insert: code }
  });
}

// ── Run ──────────────────────────────────────────────────────────────────────

async function runCode() {
  if (!wasmModule) {
    showOutput("", "T81VM is still loading. Please wait.", false);
    return;
  }

  const source = getEditorCode();
  if (!source.trim()) return;

  setStatus("run", "Running…");
  setRunButtonEnabled(false);

  // Yield to browser so the UI can update before blocking computation.
  await new Promise(r => setTimeout(r, 20));

  let result;
  try {
    const json = wasmModule.t81_run(source);
    result = JSON.parse(json);
  } catch (e) {
    result = { ok: false, output: "", errors: "Internal error: " + String(e) };
  }

  setRunButtonEnabled(true);

  const output = (result.output || "").replace(/\n$/, "");
  const errors = (result.errors || "").trim();

  if (result.ok) {
    setStatus("ok", "Done");
    showOutput(output, errors, true);
  } else {
    setStatus("err", "Error");
    showOutput(output, errors, false);
  }
}

// ── Output display ───────────────────────────────────────────────────────────

function showOutput(stdout, stderr, ok) {
  const outEl = document.getElementById("output-content");
  const errEl = document.getElementById("errors-content");

  // stdout tab
  outEl.className = "tab-content";
  if (stdout) {
    outEl.textContent = stdout;
    outEl.classList.add(ok ? "output-ok" : "output-err");
  } else if (ok) {
    outEl.textContent = "(no output)";
    outEl.classList.add("output-none");
  } else {
    outEl.textContent = "";
  }

  // errors tab
  errEl.innerHTML = "";
  if (stderr) {
    errEl.innerHTML = highlightDiagnostics(stderr);
    switchTab("errors");
  } else {
    errEl.textContent = "(no diagnostics)";
    errEl.className = "tab-content output-none";
    if (ok) switchTab("output");
  }
}

function highlightDiagnostics(text) {
  return text
    .split("\n")
    .map(line => {
      const safe = escHtml(line);
      if (/error|failed|illegal/i.test(line)) return `<span class="err-line">${safe}</span>`;
      if (/warning/i.test(line))              return `<span class="warn-line">${safe}</span>`;
      return `<span class="info-line">${safe}</span>`;
    })
    .join("\n");
}

function escHtml(s) {
  return s.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;");
}

// ── Tabs ─────────────────────────────────────────────────────────────────────

function switchTab(name) {
  activeTab = name;
  document.querySelectorAll(".tab-btn").forEach(b => {
    b.classList.toggle("active", b.dataset.tab === name);
  });
  document.querySelectorAll(".tab-content").forEach(c => {
    c.classList.toggle("active", c.id === name + "-content");
  });
}

// ── Status ───────────────────────────────────────────────────────────────────

function setStatus(state, text) {
  const dot = document.getElementById("status-dot");
  const lbl = document.getElementById("status-text");
  dot.className = "status-dot " + state;
  lbl.textContent = text;
}

function setRunButtonEnabled(enabled) {
  document.getElementById("run-btn").disabled = !enabled;
}

// ── Sample selector ───────────────────────────────────────────────────────────

function populateSamples() {
  const sel = document.getElementById("sample-select");
  Object.entries(SAMPLES).forEach(([key, { label }]) => {
    const opt = document.createElement("option");
    opt.value = key;
    opt.textContent = label;
    sel.appendChild(opt);
  });
}

function loadSample(key) {
  const s = SAMPLES[key];
  if (!s) return;
  setEditorCode(s.code);
  setStatus("idle", "Ready");
  // Clear output
  document.getElementById("output-content").textContent = "";
  document.getElementById("output-content").className = "tab-content active";
  document.getElementById("errors-content").textContent = "";
  switchTab("output");
}

// ── Resize handle ─────────────────────────────────────────────────────────────

function initResize() {
  const handle = document.getElementById("resize-handle");
  const editorPane = document.querySelector(".editor-pane");
  const workspace = document.querySelector(".workspace");
  if (!handle || !editorPane) return;

  let dragging = false;
  let startX = 0;
  let startWidth = 0;

  handle.addEventListener("mousedown", e => {
    dragging = true;
    startX = e.clientX;
    startWidth = editorPane.getBoundingClientRect().width;
    document.body.style.cursor = "col-resize";
    document.body.style.userSelect = "none";
  });

  window.addEventListener("mousemove", e => {
    if (!dragging) return;
    const totalW = workspace.getBoundingClientRect().width;
    const newW = Math.max(250, Math.min(startWidth + e.clientX - startX, totalW - 250));
    editorPane.style.flex = `0 0 ${newW}px`;
  });

  window.addEventListener("mouseup", () => {
    if (!dragging) return;
    dragging = false;
    document.body.style.cursor = "";
    document.body.style.userSelect = "";
  });
}

// ── Bootstrap ─────────────────────────────────────────────────────────────────

document.addEventListener("DOMContentLoaded", () => {
  populateSamples();
  initEditor(SAMPLES["hello_world"].code);
  initResize();

  // Tab buttons
  document.querySelectorAll(".tab-btn").forEach(btn => {
    btn.addEventListener("click", () => switchTab(btn.dataset.tab));
  });
  switchTab("output");

  // Sample selector
  document.getElementById("sample-select").addEventListener("change", e => {
    loadSample(e.target.value);
  });

  // Run button
  document.getElementById("run-btn").addEventListener("click", runCode);

  // Clear button
  document.getElementById("clear-btn").addEventListener("click", () => {
    document.getElementById("output-content").textContent = "";
    document.getElementById("output-content").className = "tab-content active";
    document.getElementById("errors-content").textContent = "";
    setStatus("idle", "Ready");
    switchTab("output");
  });

  // Keyboard shortcut: Ctrl+Enter / Cmd+Enter to run
  window.addEventListener("keydown", e => {
    if ((e.ctrlKey || e.metaKey) && e.key === "Enter") {
      e.preventDefault();
      runCode();
    }
  });

  // Load Wasm
  loadWasm();
});
