#!/usr/bin/env python3
"""Build signed generator-level WW shoving-test tables and plots.

This script only reads already-generated Pythia WW ROOT files and the existing
16-chunk cumulant summary outputs.  It does not generate events.
"""

from __future__ import annotations

import csv
import math
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import uproot


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "WW200_Generator_SignedShovingTest"
PLOTS = OUT / "plots"

FONT_SMALL = 22
FONT_MEDIUM = 24
FONT_LARGE = 28
MARKER_SIZE = 9.0
LINE_WIDTH = 2.4
CAP_SIZE = 4.5

plt.rcParams.update({
    "font.size": FONT_SMALL,
    "axes.labelsize": FONT_MEDIUM,
    "axes.titlesize": FONT_LARGE,
    "xtick.labelsize": FONT_SMALL,
    "ytick.labelsize": FONT_SMALL,
    "legend.fontsize": FONT_MEDIUM,
    "figure.titlesize": FONT_LARGE,
})

SAMPLES = [
    {
        "sample": "Pythia WW no shoving",
        "setting": "no_shoving",
        "root": ROOT / "output/pythia_noshoving_ww200_5M.root",
        "prefix": ROOT / "output/pythia_noshoving_ww200_5M",
        "plot_label": "no shoving",
    },
    {
        "sample": "Pythia WW nominal shoving",
        "setting": "nominal_shoving",
        "root": ROOT / "output/pythia_shoving_ww200_5M.root",
        "prefix": ROOT / "output/pythia_shoving_ww200_5M",
        "plot_label": "nominal shoving",
    },
    {
        "sample": "Pythia WW enhanced shoving",
        "setting": "enhanced_shoving",
        "root": ROOT / "output/pythia_shoving2x_ww200_5M.root",
        "prefix": ROOT / "output/pythia_shoving2x_ww200_5M",
        "plot_label": "enhanced shoving",
    },
]

BINS = [(0, 10), (10, 15), (15, 20), (20, 25), (25, 30), (30, 35), (35, 40), (40, 999)]
BIN_LABELS = [f"{a}_{b}" for a, b in BINS]
BIN_CENTERS = np.array([(a + b) / 2 if b < 999 else 42.5 for a, b in BINS], dtype=float)
BIN_TEXT = [f"[{a},{'inf' if b == 999 else b})" for a, b in BINS]

SIGNED_C2_CASES = [
    ("inclusive", "inclusive", 0.0, "pt04", "hC2_2", "hV2_2", "hEventCount", "hSumDen2"),
    ("rapidity_gap", "abs_delta_eta_gt_1p0", 1.0, "nonflow_etagap1p0_twosub1p0", "hCorr2EtaGap", "hV2_2EtaGap", "hEtaGapPairCapableCount", "hSumDen2EtaGap"),
    ("rapidity_gap", "abs_delta_eta_gt_1p6", 1.6, "nonflow_etagap1p6_twosub1p6", "hCorr2EtaGap", "hV2_2EtaGap", "hEtaGapPairCapableCount", "hSumDen2EtaGap"),
    ("rapidity_gap", "abs_delta_eta_gt_2p0", 2.0, "nonflow_etagap2p0_twosub2p0", "hCorr2EtaGap", "hV2_2EtaGap", "hEtaGapPairCapableCount", "hSumDen2EtaGap"),
    ("rapidity_gap", "abs_delta_eta_gt_2p5", 2.5, "nonflow_etagap2p5", "hCorr2EtaGap", "hV2_2EtaGap", "hEtaGapPairCapableCount", "hSumDen2EtaGap"),
    ("rapidity_gap", "abs_delta_eta_gt_3p0", 3.0, "nonflow_etagap3p0", "hCorr2EtaGap", "hV2_2EtaGap", "hEtaGapPairCapableCount", "hSumDen2EtaGap"),
    ("two_subevent", "eta_lt_minus_0p0_vs_eta_gt_0p0", 0.0, "pt04", "hC2TwoSub_2", "hV2TwoSub_2", "hTwoSubPairCapableCount", "hSumDen2TwoSub"),
    ("two_subevent", "eta_lt_minus_0p5_vs_eta_gt_0p5", 0.5, "nonflow_etagap1p0_twosub1p0", "hC2TwoSub_2", "hV2TwoSub_2", "hTwoSubPairCapableCount", "hSumDen2TwoSub"),
    ("two_subevent", "eta_lt_minus_0p8_vs_eta_gt_0p8", 0.8, "nonflow_etagap1p6_twosub1p6", "hC2TwoSub_2", "hV2TwoSub_2", "hTwoSubPairCapableCount", "hSumDen2TwoSub"),
    ("two_subevent", "eta_lt_minus_1p0_vs_eta_gt_1p0", 1.0, "nonflow_etagap2p0_twosub2p0", "hC2TwoSub_2", "hV2TwoSub_2", "hTwoSubPairCapableCount", "hSumDen2TwoSub"),
]


def case_paths(sample: dict, case_tag: str) -> tuple[Path, Path]:
    stem = sample["prefix"]
    return Path(f"{stem}_{case_tag}_summary.root"), Path(f"{stem}_{case_tag}_merged.root")


def hist_values(path: Path, name: str):
    with uproot.open(path) as f:
        h = f[name]
        values = np.asarray(h.values(), dtype=float)
        errors = np.asarray(h.errors(), dtype=float)
        labels = list(h.axis().labels())
    return values, errors, labels


def hist_values_noerr(path: Path, name: str):
    with uproot.open(path) as f:
        h = f[name]
        return np.asarray(h.values(), dtype=float)


def write_signed_c2_summary() -> list[dict]:
    rows = []
    for sample in SAMPLES:
        for observable_type, case_name, gap, case_tag, c2_base, v2_base, event_base, pair_base in SIGNED_C2_CASES:
            summary_path, merged_path = case_paths(sample, case_tag)
            if not summary_path.exists() or not merged_path.exists():
                raise FileNotFoundError(f"Missing {summary_path} or {merged_path}")
            for axis in ("beam", "thrust"):
                c2, c2err, labels = hist_values(summary_path, f"{c2_base}_{axis}")
                try:
                    v2, _, _ = hist_values(summary_path, f"{v2_base}_{axis}")
                except Exception:
                    v2 = np.full_like(c2, np.nan)
                events = hist_values_noerr(merged_path, f"{event_base}_{axis}")
                pairs = hist_values_noerr(merged_path, f"{pair_base}_{axis}")
                for i, label in enumerate(labels):
                    value = float(c2[i])
                    err = float(c2err[i])
                    v22 = math.sqrt(value) if value > 0.0 else math.nan
                    note = "negative c2; v2{2} magnitude not defined" if value < 0.0 else ""
                    rows.append({
                        "sample": sample["sample"],
                        "shoving_setting": sample["setting"],
                        "axis": axis,
                        "observable_type": observable_type,
                        "case": case_name,
                        "gap_value": gap,
                        "multiplicity_bin": label,
                        "multiplicity_bin_text": BIN_TEXT[i],
                        "number_of_events": float(events[i]),
                        "number_of_valid_pairs": float(pairs[i]),
                        "signed_c2_2": value,
                        "statistical_uncertainty": err,
                        "v2_2_if_c2_positive": v22,
                        "note": note,
                    })
    out = OUT / "ww200_signed_c2_summary.csv"
    with out.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)
    return rows


def plot_signed_c2(rows: list[dict]) -> None:
    PLOTS.mkdir(parents=True, exist_ok=True)
    by_key = {}
    for row in rows:
        key = (row["axis"], row["observable_type"], row["case"])
        by_key.setdefault(key, []).append(row)

    colors = {"no_shoving": "black", "nominal_shoving": "#1f77b4", "enhanced_shoving": "#d62728"}
    markers = {"no_shoving": "o", "nominal_shoving": "s", "enhanced_shoving": "^"}
    labels = {s["setting"]: s["plot_label"] for s in SAMPLES}

    for key, key_rows in by_key.items():
        axis, observable_type, case = key
        fig, axs = plt.subplots(4, 1, figsize=(10.0, 14.5), sharex=True, constrained_layout=True)
        sample_map = {}
        for setting in labels:
            setting_rows = [r for r in key_rows if r["shoving_setting"] == setting]
            setting_rows.sort(key=lambda r: BIN_LABELS.index(r["multiplicity_bin"]))
            sample_map[setting] = setting_rows
            y = np.array([r["signed_c2_2"] for r in setting_rows], dtype=float)
            e = np.array([r["statistical_uncertainty"] for r in setting_rows], dtype=float)
            axs[0].errorbar(BIN_CENTERS, y, yerr=e, marker=markers[setting], color=colors[setting], linestyle="-", label=labels[setting], capsize=CAP_SIZE, markersize=MARKER_SIZE, linewidth=LINE_WIDTH, elinewidth=1.8, capthick=1.8)

        no = sample_map["no_shoving"]
        no_y = np.array([r["signed_c2_2"] for r in no], dtype=float)
        no_e = np.array([r["statistical_uncertainty"] for r in no], dtype=float)
        deltas = {}
        for setting, axidx, title in [("nominal_shoving", 1, r"$\Delta c_2$ nominal - no shoving"), ("enhanced_shoving", 2, r"$\Delta c_2$ enhanced - no shoving")]:
            rows_s = sample_map[setting]
            y = np.array([r["signed_c2_2"] for r in rows_s], dtype=float) - no_y
            e = np.hypot(np.array([r["statistical_uncertainty"] for r in rows_s], dtype=float), no_e)
            deltas[setting] = y
            axs[axidx].axhline(0.0, color="0.5", linewidth=0.8)
            axs[axidx].errorbar(BIN_CENTERS, y, yerr=e, marker=markers[setting], color=colors[setting], linestyle="-", capsize=CAP_SIZE, markersize=MARKER_SIZE, linewidth=LINE_WIDTH, elinewidth=1.8, capthick=1.8)
            axs[axidx].set_ylabel(title)

        ratio = np.divide(deltas["enhanced_shoving"], deltas["nominal_shoving"], out=np.full_like(deltas["enhanced_shoving"], np.nan), where=np.abs(deltas["nominal_shoving"]) > 1e-15)
        axs[3].axhline(1.0, color="0.5", linewidth=0.8)
        axs[3].plot(BIN_CENTERS, ratio, marker="D", color="#9467bd", linestyle="-", markersize=MARKER_SIZE, linewidth=LINE_WIDTH)
        axs[3].set_ylabel("enhanced response\n/ nominal response", labelpad=8)
        axs[3].set_xlabel(r"$N_{trk}^{offline}$")
        axs[0].set_ylabel(r"signed $c_2\{2\}$")
        axs[0].legend(frameon=False, fontsize=FONT_MEDIUM, ncol=1, handlelength=1.3)
        for ax in axs:
            ax.grid(True, alpha=0.25)
            ax.tick_params(axis="both", which="major", labelsize=FONT_SMALL, length=6, width=1.4)
            ax.set_xticks(BIN_CENTERS)
            ax.set_xticklabels(BIN_TEXT, rotation=30, ha="right", fontsize=FONT_SMALL)
        fig.suptitle(f"WW 200 GeV signed c2, {axis} axis, {observable_type}: {case}", fontsize=FONT_LARGE)
        outbase = PLOTS / f"signed_c2_{axis}_{observable_type}_{case}"
        fig.savefig(outbase.with_suffix(".png"), dpi=160)
        fig.savefig(outbase.with_suffix(".pdf"))
        plt.close(fig)


def signed_sqrt(value: float) -> float:
    if not math.isfinite(value):
        return math.nan
    return math.copysign(math.sqrt(abs(value)), value)


def combine_vndelta_raw() -> list[dict]:
    raw_dir = OUT / "vndelta_raw"
    paths = sorted(raw_dir.glob("*_block_*.csv"))
    if not paths:
        return []

    raw_rows = []
    for path in paths:
        with path.open(newline="") as handle:
            raw_rows.extend(csv.DictReader(handle))

    grouped = {}
    for row in raw_rows:
        key = (
            row["sample"], row["shoving_setting"], row["axis"],
            row["bin_scheme"], row["multiplicity_bin"],
            row["multiplicity_bin_text"], int(row["n"]),
        )
        grouped.setdefault(key, []).append({
            "block": int(row["block"]),
            "events": float(row["number_of_events"]),
            "pairs": float(row["number_of_valid_pairs"]),
            "num": float(row["numerator"]),
        })

    rows = []
    for key, blocks in sorted(grouped.items()):
        sample, setting, axis, scheme, mult_bin, mult_text, harmonic = key
        blocks.sort(key=lambda b: b["block"])
        total_events = sum(b["events"] for b in blocks)
        total_pairs = sum(b["pairs"] for b in blocks)
        total_num = sum(b["num"] for b in blocks)
        central = total_num / total_pairs if total_pairs > 0.0 else math.nan
        leave_one = []
        for skip in range(len(blocks)):
            den = sum(b["pairs"] for i, b in enumerate(blocks) if i != skip)
            num = sum(b["num"] for i, b in enumerate(blocks) if i != skip)
            leave_one.append(num / den if den > 0.0 else math.nan)
        finite = [x for x in leave_one if math.isfinite(x)]
        if len(finite) >= 2:
            mean = sum(finite) / len(finite)
            err = math.sqrt((len(finite) - 1) / len(finite) * sum((x - mean) ** 2 for x in finite))
        else:
            err = math.nan
        rows.append({
            "sample": sample,
            "shoving_setting": setting,
            "axis": axis,
            "bin_scheme": scheme,
            "multiplicity_bin": mult_bin,
            "multiplicity_bin_text": mult_text,
            "n": harmonic,
            "number_of_events": total_events,
            "number_of_valid_pairs": total_pairs,
            "VnDelta": central,
            "statistical_uncertainty": err,
            "signed_vn_proxy": signed_sqrt(central),
        })

    out = OUT / "ww200_signed_VnDelta_summary.csv"
    with out.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)
    return rows


def plot_vndelta(rows: list[dict]) -> None:
    if not rows:
        return
    rows = [r for r in rows if r["n"] == 2 and r["bin_scheme"] in ("standard", "high_multiplicity")]
    colors = {"no_shoving": "black", "nominal_shoving": "#1f77b4", "enhanced_shoving": "#d62728"}
    markers = {"no_shoving": "o", "nominal_shoving": "s", "enhanced_shoving": "^"}
    labels = {s["setting"]: s["plot_label"] for s in SAMPLES}

    for scheme in sorted({r["bin_scheme"] for r in rows}):
        scheme_rows = [r for r in rows if r["bin_scheme"] == scheme]
        bins = sorted({r["multiplicity_bin"] for r in scheme_rows}, key=lambda x: int(x.split("_")[0]))
        centers = np.array([(int(b.split("_")[0]) + (int(b.split("_")[1]) if int(b.split("_")[1]) < 999 else int(b.split("_")[0]) + 5)) / 2 for b in bins], dtype=float)
        text_labels = [next(r["multiplicity_bin_text"] for r in scheme_rows if r["multiplicity_bin"] == b) for b in bins]
        sample_map = {}

        fig, axs = plt.subplots(5, 1, figsize=(10.0, 17.2), sharex=True, constrained_layout=True)
        for setting in labels:
            setting_rows = [r for r in scheme_rows if r["shoving_setting"] == setting]
            setting_rows.sort(key=lambda r: bins.index(r["multiplicity_bin"]))
            if not setting_rows:
                continue
            sample_map[setting] = setting_rows
            y = np.array([r["VnDelta"] for r in setting_rows], dtype=float)
            e = np.array([r["statistical_uncertainty"] for r in setting_rows], dtype=float)
            proxy = np.array([r["signed_vn_proxy"] for r in setting_rows], dtype=float)
            proxy_err = np.array([0.5 * er / math.sqrt(abs(v)) if abs(v) > 0 else math.nan for v, er in zip(y, e)], dtype=float)
            axs[0].errorbar(centers, y, yerr=e, marker=markers[setting], color=colors[setting], linestyle="-", label=labels[setting], capsize=CAP_SIZE, markersize=MARKER_SIZE, linewidth=LINE_WIDTH, elinewidth=1.8, capthick=1.8)
            axs[1].errorbar(centers, proxy, yerr=proxy_err, marker=markers[setting], color=colors[setting], linestyle="-", capsize=CAP_SIZE, markersize=MARKER_SIZE, linewidth=LINE_WIDTH, elinewidth=1.8, capthick=1.8)

        if not all(setting in sample_map for setting in ("no_shoving", "nominal_shoving", "enhanced_shoving")):
            plt.close(fig)
            continue

        no_y = np.array([r["VnDelta"] for r in sample_map["no_shoving"]], dtype=float)
        no_e = np.array([r["statistical_uncertainty"] for r in sample_map["no_shoving"]], dtype=float)
        deltas = {}
        for setting, axidx, title in [("nominal_shoving", 2, r"$\Delta V_{2\Delta}$ nominal - no shoving"), ("enhanced_shoving", 3, r"$\Delta V_{2\Delta}$ enhanced - no shoving")]:
            y = np.array([r["VnDelta"] for r in sample_map[setting]], dtype=float) - no_y
            e = np.hypot(np.array([r["statistical_uncertainty"] for r in sample_map[setting]], dtype=float), no_e)
            deltas[setting] = y
            axs[axidx].axhline(0.0, color="0.5", linewidth=0.8)
            axs[axidx].errorbar(centers, y, yerr=e, marker=markers[setting], color=colors[setting], linestyle="-", capsize=CAP_SIZE, markersize=MARKER_SIZE, linewidth=LINE_WIDTH, elinewidth=1.8, capthick=1.8)
            axs[axidx].set_ylabel(title)

        signed_delta_proxy = np.array([signed_sqrt(v) for v in deltas["enhanced_shoving"]], dtype=float)
        axs[4].axhline(0.0, color="0.5", linewidth=0.8)
        axs[4].plot(centers, signed_delta_proxy, marker="D", color="#9467bd", linestyle="-", markersize=MARKER_SIZE, linewidth=LINE_WIDTH)
        axs[4].set_ylabel(r"sign($\Delta V_{2\Delta}^{enh}$)$\sqrt{|\Delta V_{2\Delta}^{enh}|}$", labelpad=8)

        axs[0].set_ylabel(r"$V_{2\Delta}$")
        axs[1].set_ylabel(r"$v_2^{sgn}$")
        axs[0].legend(frameon=False, fontsize=FONT_MEDIUM, handlelength=1.3)
        for ax in axs:
            ax.grid(True, alpha=0.25)
            ax.tick_params(axis="both", which="major", labelsize=FONT_SMALL, length=6, width=1.4)
            ax.set_xticks(centers)
            ax.set_xticklabels(text_labels, rotation=30, ha="right", fontsize=FONT_SMALL)
        axs[-1].set_xlabel(r"$N_{trk}^{offline}$")
        fig.suptitle(f"WW 200 GeV long-range thrust-axis V2Delta, {scheme} bins", fontsize=FONT_LARGE)
        outbase = PLOTS / f"signed_V2Delta_thrust_{scheme}"
        fig.savefig(outbase.with_suffix(".png"), dpi=160)
        fig.savefig(outbase.with_suffix(".pdf"))
        plt.close(fig)


def main():
    PLOTS.mkdir(parents=True, exist_ok=True)
    rows = write_signed_c2_summary()
    plot_signed_c2(rows)
    vndelta_rows = combine_vndelta_raw()
    plot_vndelta(vndelta_rows)
    print(f"Wrote {OUT / 'ww200_signed_c2_summary.csv'}")
    print(f"Wrote signed c2 plots under {PLOTS}")
    if vndelta_rows:
        print(f"Wrote {OUT / 'ww200_signed_VnDelta_summary.csv'}")
        print(f"Wrote signed VnDelta plots under {PLOTS}")


if __name__ == "__main__":
    main()
