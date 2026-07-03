export interface CompileResult {
  ast: string;
  assembly: string;
  error: string;
}

interface EmscriptenModule {
  ccall: (name: string, returnType: string, argTypes: string[], args: unknown[]) => unknown;
  cwrap: (name: string, returnType: string, argTypes: string[]) => (...args: unknown[]) => unknown;
  UTF8ToString: (ptr: number) => string;
}

declare global {
  interface Window {
    createCompiler?: () => Promise<EmscriptenModule>;
  }
}

let compileCached: ((source: string) => CompileResult) | null = null;

export async function initCompiler(): Promise<void> {
  if (compileCached) return;

  if (typeof window.createCompiler !== "function") {
    throw new Error("Compiler module not loaded.");
  }

  const Module = await window.createCompiler();

  compileCached = (source: string): CompileResult => {
    const ptr = Module.ccall("compile_code", "number", ["string"], [source]) as number;
    const jsonStr = Module.UTF8ToString(ptr);
    const result: CompileResult = JSON.parse(jsonStr);
    Module.ccall("free_allocated", "number", ["number"], [ptr]);
    return result;
  };
}

export function compileCode(source: string): CompileResult {
  if (!compileCached) {
    return { ast: "", assembly: "", error: "Compiler not initialized." };
  }
  try {
    return compileCached(source);
  } catch (e) {
    return { ast: "", assembly: "", error: String(e) };
  }
}
