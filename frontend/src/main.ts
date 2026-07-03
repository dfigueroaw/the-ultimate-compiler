import "./style.css";
import { createEditor, getEditorCode } from "./editor";
import { initCompiler, compileCode } from "./wasm";
import { makeHorizontalResize, makeVerticalResize } from "./resize";
import { parseAst, renderAstTree, zoomAstIn, zoomAstOut, resetAstZoom } from "./ast";

const editor = createEditor(document.getElementById("editor-container")!);

const outputPane = document.getElementById("output-content")!;
const asmPane = document.getElementById("asm-content")!;
const inputContent = document.getElementById("input-content") as HTMLTextAreaElement;

const compileBtn = document.getElementById("compile-btn") as HTMLButtonElement;
const runBtn = document.getElementById("run-btn") as HTMLButtonElement;
const astBtn = document.getElementById("ast-btn") as HTMLButtonElement;
const astModal = document.getElementById("ast-modal")!;
const astContainer = document.getElementById("ast-container")!;
const astModalClose = document.getElementById("ast-modal-close")!;
const astZoomIn = document.getElementById("ast-zoom-in")!;
const astZoomOut = document.getElementById("ast-zoom-out")!;
const astZoomReset = document.getElementById("ast-zoom-reset")!;

let lastAst: string | null = null;

/* ── Resize handles ─────────────────────── */

const editorPanel = document.getElementById("editor-panel")!;
const resultsPanel = document.getElementById("results")!;
const hResize = document.querySelector(".resize-h") as HTMLElement;
makeHorizontalResize(hResize, editorPanel, resultsPanel, 200, 300);

const resultsContainer = document.getElementById("results")!;
const vHandles = document.querySelectorAll(".resize-v");
vHandles.forEach((handle, i) => {
  makeVerticalResize(handle as HTMLElement, resultsContainer, i);
});

/* ── Copy buttons ───────────────────────── */

document.querySelectorAll(".copy-btn").forEach((btn) => {
  btn.addEventListener("click", async () => {
    const targetId = (btn as HTMLElement).dataset.target;
    if (!targetId) return;
    const el = document.getElementById(targetId);
    if (!el || !el.textContent) return;
    try {
      await navigator.clipboard.writeText(el.textContent);
      btn.classList.add("copied");
      setTimeout(() => btn.classList.remove("copied"), 1500);
    } catch {
      // fallback
    }
  });
});

/* ── AST Modal ──────────────────────────── */

function showAstModal() {
  if (lastAst !== null) {
    astModal.classList.remove("hidden");
    const root = parseAst(lastAst);
    astContainer.innerHTML = "";
    requestAnimationFrame(() => renderAstTree(astContainer, root));
  }
}

function hideAstModal() {
  astModal.classList.add("hidden");
}

astBtn.addEventListener("click", showAstModal);
astModalClose.addEventListener("click", hideAstModal);
astModal.addEventListener("click", (e) => {
  if (e.target === astModal) hideAstModal();
});
document.addEventListener("keydown", (e) => {
  if (e.key === "Escape") hideAstModal();
});

astZoomIn.addEventListener("click", zoomAstIn);
astZoomOut.addEventListener("click", zoomAstOut);
astZoomReset.addEventListener("click", resetAstZoom);

/* ── Compile / Run ──────────────────────── */

function setPaneContent(pane: HTMLElement, content: string) {
  pane.classList.remove("placeholder");
  pane.textContent = content || " ";
}

function setPlaceholder(pane: HTMLElement, text: string) {
  pane.classList.add("placeholder");
  pane.textContent = text;
}

async function handleCompile() {
  const code = getEditorCode(editor);

  compileBtn.disabled = true;
  compileBtn.textContent = "Compiling...";

  try {
    const result = compileCode(code);

    if (result.error) {
      setPaneContent(outputPane, result.error);
      setPlaceholder(asmPane, "Assembly will appear here");
      lastAst = null;
      astBtn.disabled = true;
    } else {
      setPaneContent(outputPane, "Compilation successful.");
      setPaneContent(asmPane, result.assembly || "(no assembly generated)");
      lastAst = result.ast || "(empty AST)";
      astBtn.disabled = false;
    }
  } catch (err) {
    setPaneContent(outputPane, String(err));
    setPlaceholder(asmPane, "Assembly will appear here");
    lastAst = null;
    astBtn.disabled = true;
  } finally {
    compileBtn.disabled = false;
    compileBtn.textContent = "Compile";
  }
}

async function handleRun() {
  const code = getEditorCode(editor);

  compileBtn.disabled = true;
  runBtn.disabled = true;
  compileBtn.textContent = "Compiling...";

  let assembly: string;

  try {
    const result = compileCode(code);

    if (result.error) {
      setPaneContent(outputPane, result.error);
      setPlaceholder(asmPane, "Assembly will appear here");
      lastAst = null;
      astBtn.disabled = true;
      return;
    }

    assembly = result.assembly || "";
    setPaneContent(asmPane, assembly);
    lastAst = result.ast || "(empty AST)";
    astBtn.disabled = false;
  } catch (err) {
    setPaneContent(outputPane, String(err));
    return;
  } finally {
    compileBtn.disabled = false;
    runBtn.disabled = false;
    compileBtn.textContent = "Compile";
  }

  setPaneContent(outputPane, "Running...");

  try {
    const stdin = inputContent.value || "";
    const resp = await fetch("/api/run", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ assembly, stdin }),
    });

    if (!resp.ok) {
      const errText = await resp.text();
      setPaneContent(outputPane, `Run server error: ${errText}`);
      return;
    }

    const result = await resp.json();
    let output = "";
    if (result.stdout) output += result.stdout;
    if (result.stderr) {
      if (output) output += "\n";
      output += result.stderr;
    }
    if (result.exitCode !== 0) {
      if (output) output += "\n";
      output += `\nProcess exited with code ${result.exitCode}`;
    }
    if (!output) output = "(no output)";

    setPaneContent(outputPane, output);
  } catch (err) {
    setPaneContent(outputPane, `Run failed: ${err}`);
  }
}

async function init() {
  compileBtn.disabled = true;
  compileBtn.textContent = "Loading WASM...";
  runBtn.disabled = true;
  astBtn.disabled = true;

  try {
    await initCompiler();
    compileBtn.disabled = false;
    runBtn.disabled = false;
    compileBtn.textContent = "Compile";
    compileBtn.addEventListener("click", handleCompile);
    runBtn.addEventListener("click", handleRun);
  } catch (err) {
    compileBtn.textContent = "WASM unavailable";
    runBtn.textContent = "WASM unavailable";
    setPaneContent(outputPane, `Failed to load compiler: ${err}`);
    console.error("Compiler init failed:", err);
  }
}

init();
