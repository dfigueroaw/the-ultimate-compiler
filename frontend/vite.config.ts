import { defineConfig } from "vite";
import { execSync } from "child_process";
import { mkdtempSync, writeFileSync, rmSync } from "fs";
import { join } from "path";
import { tmpdir } from "os";

export default defineConfig({
  root: ".",
  build: {
    outDir: "dist",
  },
  plugins: [
    {
      name: "run-api",
      configureServer(server) {
        server.middlewares.use("/api/run", (req, res) => {
          if (req.method !== "POST") {
            res.statusCode = 405;
            res.end("Method Not Allowed");
            return;
          }

          let body = "";
          req.on("data", (chunk) => (body += chunk));
          req.on("end", () => {
            try {
              const { assembly, stdin } = JSON.parse(body);
              const result = runAssembly(assembly, stdin || "");
              res.setHeader("Content-Type", "application/json");
              res.end(JSON.stringify(result));
            } catch (e: any) {
              res.statusCode = 500;
              res.end(JSON.stringify({ stdout: "", stderr: String(e), exitCode: 1 }));
            }
          });
        });
      },
    },
  ],
});

function runAssembly(assembly: string, stdin: string) {
  const tmpDir = mkdtempSync(join(tmpdir(), "cx-run-"));
  const sFile = join(tmpDir, "program.s");
  const outFile = join(tmpDir, "program");

  writeFileSync(sFile, assembly);

  try {
    execSync(`gcc -no-pie "${sFile}" -o "${outFile}"`, {
      stdio: "pipe",
      timeout: 10000,
    });
  } catch (e: any) {
    const stderr = e.stderr?.toString() || e.message || "Assembly failed";
    rmSync(tmpDir, { recursive: true, force: true });
    return { stdout: "", stderr, exitCode: 1 };
  }

  try {
    const out = execSync(`"${outFile}"`, {
      input: stdin,
      timeout: 5000,
    });
    const stdout = out?.toString() || "";
    rmSync(tmpDir, { recursive: true, force: true });
    return { stdout, stderr: "", exitCode: 0 };
  } catch (e: any) {
    const stdout = e.stdout?.toString() || "";
    const stderr = e.stderr?.toString() || "";
    const exitCode = e.status ?? 1;
    rmSync(tmpDir, { recursive: true, force: true });
    return { stdout, stderr, exitCode };
  }
}
