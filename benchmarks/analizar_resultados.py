#!/usr/bin/env python3
"""analizar_resultados.py — genera gráficos y tablas a partir del CSV."""
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

RESULTS_DIR = Path(__file__).resolve().parent / "results"
PLOTS_DIR = Path(__file__).resolve().parent / "plots"
PLOTS_DIR.mkdir(exist_ok=True)

df = pd.read_csv(RESULTS_DIR / "resumen.csv")
df["etiqueta"] = df["compilador"] + " (" + df["nivel_opt"] + ")"

# --- Gráfico 1: tiempo de compilación por programa (large) ---
for tamano in df["tamano"].unique():
    sub = df[df["tamano"] == tamano]
    pivot = sub.pivot(index="programa", columns="etiqueta", values="tiempo_compilacion_s")
    ax = pivot.plot(kind="bar", figsize=(11, 6))
    ax.set_ylabel("Tiempo de compilación (s)")
    ax.set_title(f"Tiempo de compilación — {tamano}")
    plt.xticks(rotation=30, ha="right")
    plt.tight_layout()
    plt.savefig(PLOTS_DIR / f"compilacion_{tamano}.png", dpi=150)
    plt.close()

# --- Gráfico 2: tiempo de ejecución por programa ---
for tamano in df["tamano"].unique():
    sub = df[df["tamano"] == tamano]
    pivot = sub.pivot(index="programa", columns="etiqueta", values="mediana_ejecucion_s")
    ax = pivot.plot(kind="bar", figsize=(11, 6), logy=True)  # log-scale: útil si hay órdenes de magnitud distintos
    ax.set_ylabel("Tiempo de ejecución - mediana (s, escala log)")
    ax.set_title(f"Tiempo de ejecución — {tamano}")
    plt.xticks(rotation=30, ha="right")
    plt.tight_layout()
    plt.savefig(PLOTS_DIR / f"ejecucion_{tamano}.png", dpi=150)
    plt.close()

# --- Gráfico 3: tamaño del binario ---
for tamano in df["tamano"].unique():
    sub = df[df["tamano"] == tamano]
    pivot = sub.pivot(index="programa", columns="etiqueta", values="tamano_binario_bytes") / 1024
    ax = pivot.plot(kind="bar", figsize=(11, 6))
    ax.set_ylabel("Tamaño del binario (KB)")
    ax.set_title(f"Tamaño del binario — {tamano}")
    plt.xticks(rotation=30, ha="right")
    plt.tight_layout()
    plt.savefig(PLOTS_DIR / f"tamano_{tamano}.png", dpi=150)
    plt.close()

# --- Tabla resumen general (promedio por compilador, a través de todos los programas) ---
resumen_general = df.groupby("etiqueta").agg(
    tiempo_compilacion_prom=("tiempo_compilacion_s", "mean"),
    tiempo_ejecucion_prom=("mediana_ejecucion_s", "mean"),
    tamano_binario_prom_kb=("tamano_binario_bytes", lambda x: x.mean() / 1024),
).round(4)
print(resumen_general)
resumen_general.to_csv(RESULTS_DIR / "resumen_general.csv")

print(f"\nGráficos guardados en {PLOTS_DIR}/")
