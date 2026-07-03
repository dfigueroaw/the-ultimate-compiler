#!/usr/bin/env python3
"""
medir_optimizaciones.py — Mide el efecto de las optimizaciones propias
a nivel AST (constant folding y dead code elimination), comparando
variantes "foldable" (el optimizador puede actuar) vs "opaque" (no puede),
usando siempre el mismo binario del compilador.
"""
import subprocess
import time
import statistics
import json
import csv
import shutil
from pathlib import Path
from dataclasses import dataclass, asdict

ROOT = Path(__file__).resolve().parent
COMPILADOR = ROOT.parent / "build" / "compilador"
PROGRAMS_DIR = ROOT / "programs"
RESULTS_DIR = ROOT / "results"
WORK_DIR = ROOT / "work"
PLOTS_DIR = ROOT / "plots"
REPETICIONES = 15
TIMEOUT = 60

CASOS = ["opt_constant_folding", "opt_dead_code"]
VARIANTES = ["foldable", "opaque"]

RESULTS_DIR.mkdir(exist_ok=True)
WORK_DIR.mkdir(exist_ok=True)
PLOTS_DIR.mkdir(exist_ok=True)


@dataclass
class ResultadoOptimizacion:
    caso: str
    variante: str
    tiempo_compilacion: float
    lineas_asm: int
    tamano_binario: int
    tiempos_ejecucion: list
    media_ejecucion: float
    mediana_ejecucion: float
    stdev_ejecucion: float


def correr(cmd, timeout=TIMEOUT):
    if shutil.which(cmd[0]) is None:
        return None, None
    inicio = time.perf_counter()
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return None, None
    fin = time.perf_counter()
    return r, fin - inicio


def compilar(fuente: Path, asm_path: Path, out_bin: Path):
    r1, t1 = correr([str(COMPILADOR), str(fuente), "-o", str(asm_path)])
    if r1 is None or r1.returncode != 0:
        return None, False
    r2, t2 = correr(["gcc", "-no-pie", str(asm_path), "-o", str(out_bin)])
    if r2 is None or r2.returncode != 0:
        return None, False
    return t1 + t2, True


def medir_ejecucion(binario: Path, n=REPETICIONES):
    tiempos = []
    for _ in range(n):
        r, t = correr([str(binario)])
        if r is None or r.returncode != 0:
            continue
        tiempos.append(t)
    return tiempos


def medir_uno(caso: str, variante: str):
    fuente = PROGRAMS_DIR / caso / f"{variante}.c"
    if not fuente.exists():
        print(f"  [FALLA] no existe {fuente}")
        return None

    asm_path = WORK_DIR / f"{caso}_{variante}.s"
    out_bin = WORK_DIR / f"{caso}_{variante}"

    t_compilacion, ok = compilar(fuente, asm_path, out_bin)
    if not ok:
        print(f"  [FALLA] {caso}/{variante} no pudo compilar/ensamblar")
        return None

    lineas_asm = sum(1 for _ in open(asm_path))
    tam_binario = out_bin.stat().st_size

    tiempos = medir_ejecucion(out_bin)
    if not tiempos:
        print(f"  [FALLA] {caso}/{variante} no pudo ejecutarse")
        return None

    return ResultadoOptimizacion(
        caso=caso,
        variante=variante,
        tiempo_compilacion=t_compilacion,
        lineas_asm=lineas_asm,
        tamano_binario=tam_binario,
        tiempos_ejecucion=tiempos,
        media_ejecucion=statistics.mean(tiempos),
        mediana_ejecucion=statistics.median(tiempos),
        stdev_ejecucion=statistics.stdev(tiempos) if len(tiempos) > 1 else 0.0,
    )


def main():
    if not COMPILADOR.exists():
        print(f"[ERROR] No se encontró el compilador en {COMPILADOR}")
        print("Corre 'make' desde la raíz del proyecto primero.")
        return

    resultados = []
    for caso in CASOS:
        print(f"\n=== {caso} ===")
        for variante in VARIANTES:
            res = medir_uno(caso, variante)
            if res:
                resultados.append(res)
                print(f"  {variante}: {res.lineas_asm} líneas .s | "
                      f"{res.tamano_binario} bytes | "
                      f"mediana ejecución {res.mediana_ejecucion*1000:.4f} ms "
                      f"(stdev {res.stdev_ejecucion*1000:.4f} ms)")

    with open(RESULTS_DIR / "optimizaciones.json", "w") as f:
        json.dump([asdict(r) for r in resultados], f, indent=2)

    with open(RESULTS_DIR / "optimizaciones.csv", "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            "caso", "variante", "tiempo_compilacion_s", "lineas_asm",
            "tamano_binario_bytes", "media_ejecucion_s",
            "mediana_ejecucion_s", "stdev_ejecucion_s"
        ])
        for r in resultados:
            writer.writerow([
                r.caso, r.variante, f"{r.tiempo_compilacion:.6f}",
                r.lineas_asm, r.tamano_binario,
                f"{r.media_ejecucion:.6f}", f"{r.mediana_ejecucion:.6f}",
                f"{r.stdev_ejecucion:.6f}"
            ])

    print("\n=== Resumen del efecto de la optimización ===")
    for caso in CASOS:
        fold = next((r for r in resultados if r.caso == caso and r.variante == "foldable"), None)
        opaq = next((r for r in resultados if r.caso == caso and r.variante == "opaque"), None)
        if fold and opaq:
            print(f"{caso}:")
            print(f"  líneas .s   -> foldable {fold.lineas_asm}, opaque {opaq.lineas_asm} "
                  f"(diferencia: {opaq.lineas_asm - fold.lineas_asm} líneas)")
            print(f"  ejecución   -> foldable {fold.mediana_ejecucion*1000:.4f} ms, "
                  f"opaque {opaq.mediana_ejecucion*1000:.4f} ms "
                  f"(diferencia: {(opaq.mediana_ejecucion - fold.mediana_ejecucion)*1000:.4f} ms)")

    print(f"\nResultados guardados en {RESULTS_DIR}/optimizaciones.csv y optimizaciones.json")


if __name__ == "__main__":
    main()
