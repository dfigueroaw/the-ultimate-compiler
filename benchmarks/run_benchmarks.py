#!/usr/bin/env python3
"""
run_benchmarks.py — Compara tu_compilador vs gcc vs clang
en tiempo de compilación, tiempo de ejecución y tamaño de binario.
"""
import subprocess
import time
import statistics
import json
import csv
import os
from pathlib import Path
from dataclasses import dataclass, asdict

# ---------- Configuración ----------
ROOT = Path(__file__).resolve().parent
COMPILADOR = ROOT.parent / "build" / "compilador"   # ajusta el path
PROGRAMS_DIR = ROOT / "programs"
RESULTS_DIR = ROOT / "results"
WORK_DIR = ROOT / "work"          # binarios/asm temporales
REPETICIONES = 15                 # repeticiones por medición
TIMEOUT = 60                      # segundos, evita colgar el benchmark

RESULTS_DIR.mkdir(exist_ok=True)
WORK_DIR.mkdir(exist_ok=True)


@dataclass
class Resultado:
    programa: str
    tamano: str            # "small" / "large"
    compilador: str        # "tu_compilador" / "gcc" / "clang"
    nivel_opt: str         # "O0" / "O1"
    tiempo_compilacion: float
    tamano_binario: int
    tiempos_ejecucion: list
    media_ejecucion: float
    mediana_ejecucion: float
    stdev_ejecucion: float


def correr(cmd, timeout=TIMEOUT):
    inicio = time.perf_counter()
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return None, None
    fin = time.perf_counter()
    return r, fin - inicio


def compilar_tu_compilador(fuente: Path, out_bin: Path):
    """Devuelve (tiempo_total, ok). Incluye compilador propio + gcc -no-pie."""
    asm_path = out_bin.with_suffix(".s")
    r1, t1 = correr([str(COMPILADOR), str(fuente), "-o", str(asm_path)])
    if r1 is None or r1.returncode != 0:
        return None, False
    r2, t2 = correr(["gcc", "-no-pie", str(asm_path), "-o", str(out_bin)])
    if r2 is None or r2.returncode != 0:
        return None, False
    return t1 + t2, True


def compilar_gcc(fuente: Path, out_bin: Path, nivel: str):
    r, t = correr(["gcc", f"-{nivel}", str(fuente), "-o", str(out_bin)])
    ok = r is not None and r.returncode == 0
    return t, ok


def compilar_clang(fuente: Path, out_bin: Path, nivel: str):
    r, t = correr(["clang", f"-{nivel}", str(fuente), "-o", str(out_bin)])
    ok = r is not None and r.returncode == 0
    return t, ok


def medir_ejecucion(binario: Path, n=REPETICIONES):
    tiempos = []
    for _ in range(n):
        r, t = correr([str(binario)])
        if r is None or r.returncode != 0:
            continue
        tiempos.append(t)
    return tiempos


def benchmark_uno(programa: str, tamano: str, fuente: Path, compilador: str, nivel: str, fn_compilar):
    out_bin = WORK_DIR / f"{programa}_{tamano}_{compilador}_{nivel}"
    t_compilacion, ok = fn_compilar(fuente, out_bin)
    if not ok:
        print(f"  [FALLA] {compilador} {nivel} no pudo compilar {programa}/{tamano}")
        return None

    tam_binario = out_bin.stat().st_size
    tiempos = medir_ejecucion(out_bin)
    if not tiempos:
        print(f"  [FALLA] {compilador} {nivel} no pudo ejecutar {programa}/{tamano}")
        return None

    return Resultado(
        programa=programa,
        tamano=tamano,
        compilador=compilador,
        nivel_opt=nivel,
        tiempo_compilacion=t_compilacion,
        tamano_binario=tam_binario,
        tiempos_ejecucion=tiempos,
        media_ejecucion=statistics.mean(tiempos),
        mediana_ejecucion=statistics.median(tiempos),
        stdev_ejecucion=statistics.stdev(tiempos) if len(tiempos) > 1 else 0.0,
    )


def main():
    resultados = []
    for prog_dir in sorted(PROGRAMS_DIR.iterdir()):
        if not prog_dir.is_dir():
            continue
        programa = prog_dir.name
        for fuente in sorted(prog_dir.glob("*.c")):
            tamano = fuente.stem  # "small" o "large"
            print(f"Benchmarking {programa}/{tamano}...")

            configs = [
            ("the-ultimate-compiler", "default", compilar_tu_compilador),
            ("gcc", "O0", lambda f, o: compilar_gcc(f, o, "O0")),
            ("gcc", "O1", lambda f, o: compilar_gcc(f, o, "O1")),
            ("gcc", "O2", lambda f, o: compilar_gcc(f, o, "O2")),
            ("gcc", "O3", lambda f, o: compilar_gcc(f, o, "O3")),
            ("clang", "O0", lambda f, o: compilar_clang(f, o, "O0")),
            ("clang", "O1", lambda f, o: compilar_clang(f, o, "O1")),
            ("clang", "O2", lambda f, o: compilar_clang(f, o, "O2")),
            ("clang", "O3", lambda f, o: compilar_clang(f, o, "O3")),
        ]

            for compilador, nivel, fn in configs:
                res = benchmark_uno(programa, tamano, fuente, compilador, nivel, fn)
                if res:
                    resultados.append(res)

    # Guardar resultados crudos en JSON
    with open(RESULTS_DIR / "resultados.json", "w") as f:
        json.dump([asdict(r) for r in resultados], f, indent=2)

    # Guardar resumen en CSV (para pandas/excel)
    with open(RESULTS_DIR / "resumen.csv", "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            "programa", "tamano", "compilador", "nivel_opt",
            "tiempo_compilacion_s", "tamano_binario_bytes",
            "media_ejecucion_s", "mediana_ejecucion_s", "stdev_ejecucion_s"
        ])
        for r in resultados:
            writer.writerow([
                r.programa, r.tamano, r.compilador, r.nivel_opt,
                f"{r.tiempo_compilacion:.6f}", r.tamano_binario,
                f"{r.media_ejecucion:.6f}", f"{r.mediana_ejecucion:.6f}",
                f"{r.stdev_ejecucion:.6f}"
            ])

    print(f"\nListo. {len(resultados)} configuraciones benchmarkeadas.")
    print(f"Resultados en {RESULTS_DIR}/")


if __name__ == "__main__":
    main()
