#!/usr/bin/env python3
"""analizar_resultados.py — genera gráficos y tablas a partir de los CSV."""
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

RESULTS_DIR = Path(__file__).resolve().parent / "results"
PLOTS_DIR = Path(__file__).resolve().parent / "plots"
PLOTS_DIR.mkdir(exist_ok=True)

# ================== Benchmark principal (resumen.csv) ==================
resumen_path = RESULTS_DIR / "resumen.csv"
if resumen_path.exists():
    df = pd.read_csv(resumen_path)
    df["etiqueta"] = df["compilador"] + " (" + df["nivel_opt"] + ")"

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

        pivot = sub.pivot(index="programa", columns="etiqueta", values="mediana_ejecucion_s")
        ax = pivot.plot(kind="bar", figsize=(11, 6), logy=True)
        ax.set_ylabel("Tiempo de ejecución - mediana (s, escala log)")
        ax.set_title(f"Tiempo de ejecución — {tamano}")
        plt.xticks(rotation=30, ha="right")
        plt.tight_layout()
        plt.savefig(PLOTS_DIR / f"ejecucion_{tamano}.png", dpi=150)
        plt.close()

        pivot = sub.pivot(index="programa", columns="etiqueta", values="tamano_binario_bytes") / 1024
        ax = pivot.plot(kind="bar", figsize=(11, 6))
        ax.set_ylabel("Tamaño del binario (KB)")
        ax.set_title(f"Tamaño del binario — {tamano}")
        plt.xticks(rotation=30, ha="right")
        plt.tight_layout()
        plt.savefig(PLOTS_DIR / f"tamano_{tamano}.png", dpi=150)
        plt.close()

    resumen_general = df.groupby("etiqueta").agg(
        tiempo_compilacion_prom=("tiempo_compilacion_s", "mean"),
        tiempo_ejecucion_prom=("mediana_ejecucion_s", "mean"),
        tamano_binario_prom_kb=("tamano_binario_bytes", lambda x: x.mean() / 1024),
    ).round(4)
    print("=== Resumen general (benchmark principal) ===")
    print(resumen_general)
    resumen_general.to_csv(RESULTS_DIR / "resumen_general.csv")
else:
    print(f"[AVISO] No se encontró {resumen_path}, se omite el benchmark principal.")

# ================== Optimizaciones propias (optimizaciones.csv) ==================
opt_path = RESULTS_DIR / "optimizaciones.csv"
if opt_path.exists():
    df_opt = pd.read_csv(opt_path)

    for caso in df_opt["caso"].unique():
        sub = df_opt[df_opt["caso"] == caso].set_index("variante")

        fig, axes = plt.subplots(1, 2, figsize=(10, 4.5))

        sub["lineas_asm"].plot(kind="bar", ax=axes[0], color=["#4C72B0", "#DD8452"])
        axes[0].set_ylabel("Líneas de ensamblador")
        axes[0].set_title("Instrucciones generadas")
        axes[0].tick_params(axis="x", rotation=0)

        (sub["mediana_ejecucion_s"] * 1000).plot(kind="bar", ax=axes[1], color=["#4C72B0", "#DD8452"])
        axes[1].set_ylabel("Tiempo de ejecución (ms)")
        axes[1].set_title("Mediana de ejecución")
        axes[1].tick_params(axis="x", rotation=0)

        fig.suptitle(f"Efecto de la optimización — {caso}")
        plt.tight_layout()
        plt.savefig(PLOTS_DIR / f"optimizacion_{caso}.png", dpi=150)
        plt.close()

    print("\n=== Resumen de optimizaciones ===")
    print(df_opt[["caso", "variante", "lineas_asm", "mediana_ejecucion_s"]])
else:
    print(f"[AVISO] No se encontró {opt_path}, se omite la sección de optimizaciones.")

print(f"\nGráficos guardados en {PLOTS_DIR}/")
