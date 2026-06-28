#!/usr/bin/env python3
"""Build signed generator-level inclusive qqbar 200 GeV shoving-test tables and plots."""

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
OUT = ROOT / "LEP2QQ200_Generator_SignedShovingTest"
PLOTS = OUT / "plots"

FONT_SMALL = 26
FONT_MEDIUM = 30
FONT_LARGE = 34
MARKER_SIZE = 11.0
LINE_WIDTH = 3.0
CAP_SIZE = 5.5

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
        "sample": "Pythia qqbar 200 GeV no shoving",
        "setting": "no_shoving",
        "root": ROOT / "output/pythia_noshoving_qq200_5M.root",
        "prefix": ROOT / "output/pythia_noshoving_qq200_5M",
        "plot_label": "no shoving",
    },
    {
        "sample": "Pythia qqbar 200 GeV nominal shoving",
        "setting": "nominal_shoving",
        "root": ROOT / "output/pythia_shoving_qq200_5M.root",
        "prefix": ROOT / "output/pythia_shoving_qq200_5M",
        "plot_label": "nominal shoving",
    },
    {
        "sample": "Pythia qqbar 200 GeV enhanced shoving",
        "setting": "enhanced_shoving",
        "root": ROOT / "output/pythia_shoving2x_qq200_5M.root",
        "prefix": ROOT / "output/pythia_shoving2x_qq200_5M",
        "plot_label": "enhanced shoving",
    },
]

BINS = [(0, 10), (10, 15), (15, 20), (20, 25), (25, 30), (30, 35), (35, 40), (40, 999)]
BIN_LABELS = [f"{a}_{b}" for a, b in BINS]
BIN_CENTERS = np.array([(a + b) / 2 if b < 999 else 42.5 for a, b in BINS], dtype=float)
BIN_TEXT = [f"[{a},{'inf' if b == 999 else b})" for a, b in BINS]

SIGNED_C2_CASES = [
    ("inclusive", "inclusive", 0.0, "pt04", "hC2_2", "hV2_2", "hEventCount", "hSumDen2"),
    ("pt_selection", "pt_1_to_3_GeV", 0.0, "nonflow_pt1to3", "hC2_2", "hV2_2", "hEventCount", "hSumDen2"),
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


def decode_array(values) -> list[str]:
    result = []
    for value in values:
        if isinstance(value, bytes):
            result.append(value.decode("utf-8", errors="replace").rstrip("\x00"))
        else:
            result.append(str(value))
    return result


def write_inventory() -> None:
    lines = [
        "Inclusive LEP2 200 GeV generator sample inventory",
        "================================================",
        "",
        "Process definition: Pythia8 WeakSingleBoson:ffbar2gmZ with 23:onIfAny = 1 2 3 4 5 at sqrt(s)=200 GeV.",
        "Saved event content: final-state visible particles, excluding neutrinos, with ALEPH-like acceptance disabled.",
        "",
    ]

    for sample in SAMPLES:
        path = sample["root"]
        lines.append(f"Sample: {sample['sample']}")
        lines.append(f"file path: {path}")
        if not path.exists():
            lines.append("status: missing")
            lines.append("")
            continue

        with uproot.open(path) as handle:
            keys = [key.split(";")[0] for key in handle.keys()]
            if "t" in handle:
                tree = handle["t"]
                branches = list(tree.keys())
                entries = tree.num_entries
            else:
                tree = None
                branches = []
                entries = 0
            config = {}
            if "pythia_config" in handle:
                cfg = handle["pythia_config"]
                keys_cfg = decode_array(cfg["key"].array(library="np"))
                vals_cfg = decode_array(cfg["value"].array(library="np"))
                config = dict(zip(keys_cfg, vals_cfg))

        final_state = all(name in branches for name in ("px", "py", "pz", "energy", "pid", "pwflag", "charge"))
        vertices = all(name in branches for name in ("vx", "vy", "vz"))
        partons = any(name in branches for name in ("status", "mother1", "mother2", "daughter1", "daughter2"))
        strings = any(("string" in name.lower() or "gleipnir" in name.lower()) for name in branches)
        mother_links = any("mother" in name.lower() or "daughter" in name.lower() for name in branches)

        lines.append("tree name: t")
        lines.append(f"total number of entries: {entries}")
        lines.append("branch list: " + ", ".join(branches))
        lines.append(f"top-level ROOT objects: {', '.join(keys)}")
        lines.append(f"final-state charged particles available: {'yes' if final_state else 'no'}")
        lines.append(f"final-state production vertices available: {'yes (vx, vy, vz for saved final-state particles)' if vertices else 'no'}")
        lines.append(f"generator-level partons available: {'yes' if partons else 'no'}")
        lines.append(f"string pieces or Gleipnir space-time information available: {'yes' if strings else 'no'}")
        lines.append(f"truth W daughters or event-record mother/daughter indices available: {'yes' if mother_links else 'no'}")
        if config:
            for key in ("ECM", "ProcessName", "Process", "EnableShoving", "ShovingRepulsionFactor", "ShovingSettings"):
                lines.append(f"config {key}: {config.get(key, '')}")
        lines.append("")

    lines.append("Geometry-plane signed v2 status: skipped. The saved files contain final-state particle vertices but not the full event record, parton dipoles, string pieces, or mother/daughter links needed for a controlled generator geometry-plane definition.")
    (OUT / "qq200_generator_sample_inventory.txt").write_text("\n".join(lines) + "\n")


def case_paths(sample: dict, case_tag: str) -> tuple[Path, Path]:
    stem = sample["prefix"]
    return Path(f"{stem}_{case_tag}_summary.root"), Path(f"{stem}_{case_tag}_merged.root")


def hist_values(path: Path, name: str):
    with uproot.open(path) as handle:
        hist = handle[name]
        values = np.asarray(hist.values(), dtype=float)
        errors = np.asarray(hist.errors(), dtype=float)
        labels = list(hist.axis().labels())
    return values, errors, labels


def hist_values_noerr(path: Path, name: str):
    with uproot.open(path) as handle:
        hist = handle[name]
        return np.asarray(hist.values(), dtype=float)


def write_signed_c2_summary() -> list[dict]:
    raw_dir = OUT / "signed_c2_raw"
    raw_paths = sorted(raw_dir.glob("*_block_*.csv"))
    if raw_paths:
        return combine_signed_c2_raw(raw_paths)

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
                        "v2_2_if_c2_positive": math.sqrt(value) if value > 0.0 else math.nan,
                        "v2_2_from_summary": float(v2[i]) if i < len(v2) else math.nan,
                        "note": "negative c2; v2{2} magnitude not defined" if value < 0.0 else "",
                    })

    out = OUT / "qq200_signed_c2_summary.csv"
    with out.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)
    return rows



def combine_signed_c2_raw(paths: list[Path]) -> list[dict]:
    raw_rows = []
    for path in paths:
        with path.open(newline="") as handle:
            raw_rows.extend(csv.DictReader(handle))

    grouped = {}
    for row in raw_rows:
        key = (
            row["sample"],
            row["shoving_setting"],
            row["axis"],
            row["observable_type"],
            row["case"],
            float(row["gap_value"]),
            row["multiplicity_bin"],
            row["multiplicity_bin_text"],
        )
        grouped.setdefault(key, []).append({
            "block": int(row["block"]),
            "events": float(row["number_of_events"]),
            "pairs": float(row["number_of_valid_pairs"]),
            "num": float(row["numerator"]),
        })

    rows = []
    for key, blocks in sorted(grouped.items()):
        sample, setting, axis, observable_type, case_name, gap, mult_bin, mult_text = key
        blocks.sort(key=lambda block: block["block"])
        total_events = sum(block["events"] for block in blocks)
        total_pairs = sum(block["pairs"] for block in blocks)
        total_num = sum(block["num"] for block in blocks)
        central = total_num / total_pairs if total_pairs > 0.0 else math.nan
        leave_one = []
        for skip in range(len(blocks)):
            den = sum(block["pairs"] for idx, block in enumerate(blocks) if idx != skip)
            num = sum(block["num"] for idx, block in enumerate(blocks) if idx != skip)
            leave_one.append(num / den if den > 0.0 else math.nan)
        finite = [value for value in leave_one if math.isfinite(value)]
        if len(finite) >= 2:
            mean = sum(finite) / len(finite)
            err = math.sqrt((len(finite) - 1) / len(finite) * sum((value - mean) ** 2 for value in finite))
        else:
            err = math.nan
        rows.append({
            "sample": sample,
            "shoving_setting": setting,
            "axis": axis,
            "observable_type": observable_type,
            "case": case_name,
            "gap_value": gap,
            "multiplicity_bin": mult_bin,
            "multiplicity_bin_text": mult_text,
            "number_of_events": total_events,
            "number_of_valid_pairs": total_pairs,
            "signed_c2_2": central,
            "statistical_uncertainty": err,
            "v2_2_if_c2_positive": math.sqrt(central) if central > 0.0 else math.nan,
            "v2_2_from_summary": math.nan,
            "note": "negative c2; v2{2} magnitude not defined" if central < 0.0 else "",
        })

    out = OUT / "qq200_signed_c2_summary.csv"
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
    labels = {sample["setting"]: sample["plot_label"] for sample in SAMPLES}

    for key, key_rows in by_key.items():
        axis, observable_type, case = key
        fig, axs = plt.subplots(4, 1, figsize=(13.0, 18.0), sharex=True, constrained_layout=True)
        sample_map = {}
        for setting in labels:
            setting_rows = [row for row in key_rows if row["shoving_setting"] == setting]
            setting_rows.sort(key=lambda row: BIN_LABELS.index(row["multiplicity_bin"]))
            sample_map[setting] = setting_rows
            y = np.array([row["signed_c2_2"] for row in setting_rows], dtype=float)
            e = np.array([row["statistical_uncertainty"] for row in setting_rows], dtype=float)
            axs[0].errorbar(
                BIN_CENTERS,
                y,
                yerr=e,
                marker=markers[setting],
                color=colors[setting],
                linestyle="-",
                label=labels[setting],
                capsize=CAP_SIZE,
                markersize=MARKER_SIZE,
                linewidth=LINE_WIDTH,
                elinewidth=2.2,
                capthick=2.2,
            )

        no_y = np.array([row["signed_c2_2"] for row in sample_map["no_shoving"]], dtype=float)
        no_e = np.array([row["statistical_uncertainty"] for row in sample_map["no_shoving"]], dtype=float)
        deltas = {}
        delta_labels = [
            ("nominal_shoving", 1, r"$\Delta c_2$ nominal - no shoving"),
            ("enhanced_shoving", 2, r"$\Delta c_2$ enhanced - no shoving"),
        ]
        for setting, axidx, ylabel in delta_labels:
            y = np.array([row["signed_c2_2"] for row in sample_map[setting]], dtype=float) - no_y
            e = np.hypot(np.array([row["statistical_uncertainty"] for row in sample_map[setting]], dtype=float), no_e)
            deltas[setting] = y
            axs[axidx].axhline(0.0, color="0.45", linewidth=1.2)
            axs[axidx].errorbar(
                BIN_CENTERS,
                y,
                yerr=e,
                marker=markers[setting],
                color=colors[setting],
                linestyle="-",
                capsize=CAP_SIZE,
                markersize=MARKER_SIZE,
                linewidth=LINE_WIDTH,
                elinewidth=2.2,
                capthick=2.2,
            )
            axs[axidx].set_ylabel(ylabel)

        ratio = np.divide(
            deltas["enhanced_shoving"],
            deltas["nominal_shoving"],
            out=np.full_like(deltas["enhanced_shoving"], np.nan),
            where=np.abs(deltas["nominal_shoving"]) > 1e-15,
        )
        axs[3].axhline(1.0, color="0.45", linewidth=1.2)
        axs[3].plot(BIN_CENTERS, ratio, marker="D", color="#9467bd", linestyle="-", markersize=MARKER_SIZE, linewidth=LINE_WIDTH)
        axs[3].set_ylabel("enhanced response\n/ nominal response", labelpad=10)
        axs[3].set_xlabel(r"$N_{trk}^{offline}$")
        axs[0].set_ylabel(r"signed $c_2\{2\}$")
        axs[0].legend(frameon=False, loc="best", handlelength=1.5)
        for ax in axs:
            ax.grid(True, alpha=0.25)
            ax.tick_params(axis="both", which="major", labelsize=FONT_SMALL, length=7, width=1.6)
            ax.set_xticks(BIN_CENTERS)
            ax.set_xticklabels(BIN_TEXT, rotation=30, ha="right", fontsize=FONT_SMALL)
        fig.suptitle(f"Inclusive qqbar 200 GeV signed c2, {axis} axis, {observable_type}: {case}")
        outbase = PLOTS / f"signed_c2_{axis}_{observable_type}_{case}"
        fig.savefig(outbase.with_suffix(".png"), dpi=170)
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
            row["sample"],
            row["shoving_setting"],
            row["axis"],
            row["bin_scheme"],
            row["multiplicity_bin"],
            row["multiplicity_bin_text"],
            int(row["n"]),
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
        blocks.sort(key=lambda block: block["block"])
        total_events = sum(block["events"] for block in blocks)
        total_pairs = sum(block["pairs"] for block in blocks)
        total_num = sum(block["num"] for block in blocks)
        central = total_num / total_pairs if total_pairs > 0.0 else math.nan
        leave_one = []
        for skip in range(len(blocks)):
            den = sum(block["pairs"] for idx, block in enumerate(blocks) if idx != skip)
            num = sum(block["num"] for idx, block in enumerate(blocks) if idx != skip)
            leave_one.append(num / den if den > 0.0 else math.nan)
        finite = [value for value in leave_one if math.isfinite(value)]
        if len(finite) >= 2:
            mean = sum(finite) / len(finite)
            err = math.sqrt((len(finite) - 1) / len(finite) * sum((value - mean) ** 2 for value in finite))
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

    out = OUT / "qq200_signed_VnDelta_summary.csv"
    with out.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)
    return rows


def plot_vndelta(rows: list[dict]) -> None:
    if not rows:
        return
    rows = [row for row in rows if row["n"] == 2 and row["axis"] == "thrust" and row["bin_scheme"] in ("standard", "high_multiplicity")]
    colors = {"no_shoving": "black", "nominal_shoving": "#1f77b4", "enhanced_shoving": "#d62728"}
    markers = {"no_shoving": "o", "nominal_shoving": "s", "enhanced_shoving": "^"}
    labels = {sample["setting"]: sample["plot_label"] for sample in SAMPLES}

    for scheme in sorted({row["bin_scheme"] for row in rows}):
        scheme_rows = [row for row in rows if row["bin_scheme"] == scheme]
        bins = sorted({row["multiplicity_bin"] for row in scheme_rows}, key=lambda value: int(value.split("_")[0]))
        centers = np.array(
            [
                (int(bin_label.split("_")[0]) + (int(bin_label.split("_")[1]) if int(bin_label.split("_")[1]) < 999 else int(bin_label.split("_")[0]) + 5)) / 2
                for bin_label in bins
            ],
            dtype=float,
        )
        text_labels = [next(row["multiplicity_bin_text"] for row in scheme_rows if row["multiplicity_bin"] == bin_label) for bin_label in bins]
        sample_map = {}

        fig, axs = plt.subplots(5, 1, figsize=(13.0, 21.0), sharex=True, constrained_layout=True)
        for setting in labels:
            setting_rows = [row for row in scheme_rows if row["shoving_setting"] == setting]
            setting_rows.sort(key=lambda row: bins.index(row["multiplicity_bin"]))
            if not setting_rows:
                continue
            sample_map[setting] = setting_rows
            y = np.array([row["VnDelta"] for row in setting_rows], dtype=float)
            e = np.array([row["statistical_uncertainty"] for row in setting_rows], dtype=float)
            proxy = np.array([row["signed_vn_proxy"] for row in setting_rows], dtype=float)
            proxy_err = np.array([0.5 * err / math.sqrt(abs(value)) if abs(value) > 0 else math.nan for value, err in zip(y, e)], dtype=float)
            axs[0].errorbar(centers, y, yerr=e, marker=markers[setting], color=colors[setting], linestyle="-", label=labels[setting], capsize=CAP_SIZE, markersize=MARKER_SIZE, linewidth=LINE_WIDTH, elinewidth=2.2, capthick=2.2)
            axs[1].errorbar(centers, proxy, yerr=proxy_err, marker=markers[setting], color=colors[setting], linestyle="-", capsize=CAP_SIZE, markersize=MARKER_SIZE, linewidth=LINE_WIDTH, elinewidth=2.2, capthick=2.2)

        if not all(setting in sample_map for setting in ("no_shoving", "nominal_shoving", "enhanced_shoving")):
            plt.close(fig)
            continue

        no_y = np.array([row["VnDelta"] for row in sample_map["no_shoving"]], dtype=float)
        no_e = np.array([row["statistical_uncertainty"] for row in sample_map["no_shoving"]], dtype=float)
        deltas = {}
        for setting, axidx, ylabel in [
            ("nominal_shoving", 2, r"$\Delta V_{2\Delta}$ nominal - no shoving"),
            ("enhanced_shoving", 3, r"$\Delta V_{2\Delta}$ enhanced - no shoving"),
        ]:
            y = np.array([row["VnDelta"] for row in sample_map[setting]], dtype=float) - no_y
            e = np.hypot(np.array([row["statistical_uncertainty"] for row in sample_map[setting]], dtype=float), no_e)
            deltas[setting] = y
            axs[axidx].axhline(0.0, color="0.45", linewidth=1.2)
            axs[axidx].errorbar(centers, y, yerr=e, marker=markers[setting], color=colors[setting], linestyle="-", capsize=CAP_SIZE, markersize=MARKER_SIZE, linewidth=LINE_WIDTH, elinewidth=2.2, capthick=2.2)
            axs[axidx].set_ylabel(ylabel)

        signed_delta_proxy = np.array([signed_sqrt(value) for value in deltas["enhanced_shoving"]], dtype=float)
        axs[4].axhline(0.0, color="0.45", linewidth=1.2)
        axs[4].plot(centers, signed_delta_proxy, marker="D", color="#9467bd", linestyle="-", markersize=MARKER_SIZE, linewidth=LINE_WIDTH)
        axs[4].set_ylabel(r"sign($\Delta V_{2\Delta}^{enh}$)$\sqrt{|\Delta V_{2\Delta}^{enh}|}$", labelpad=10)

        axs[0].set_ylabel(r"$V_{2\Delta}$")
        axs[1].set_ylabel(r"$v_2^{sgn}$")
        axs[0].legend(frameon=False, loc="best", handlelength=1.5)
        for ax in axs:
            ax.grid(True, alpha=0.25)
            ax.tick_params(axis="both", which="major", labelsize=FONT_SMALL, length=7, width=1.6)
            ax.set_xticks(centers)
            ax.set_xticklabels(text_labels, rotation=30, ha="right", fontsize=FONT_SMALL)
        axs[-1].set_xlabel(r"$N_{trk}^{offline}$")
        fig.suptitle(f"Inclusive qqbar 200 GeV long-range thrust-axis V2Delta, {scheme} bins")
        outbase = PLOTS / f"signed_V2Delta_thrust_{scheme}"
        fig.savefig(outbase.with_suffix(".png"), dpi=170)
        fig.savefig(outbase.with_suffix(".pdf"))
        plt.close(fig)


def main() -> None:
    PLOTS.mkdir(parents=True, exist_ok=True)
    write_inventory()
    rows = write_signed_c2_summary()
    plot_signed_c2(rows)
    vndelta_rows = combine_vndelta_raw()
    plot_vndelta(vndelta_rows)
    print(f"Wrote {OUT / 'qq200_generator_sample_inventory.txt'}")
    print(f"Wrote {OUT / 'qq200_signed_c2_summary.csv'}")
    print(f"Wrote signed c2 plots under {PLOTS}")
    if vndelta_rows:
        print(f"Wrote {OUT / 'qq200_signed_VnDelta_summary.csv'}")
        print(f"Wrote signed VnDelta plots under {PLOTS}")
    else:
        print("No VnDelta raw block CSV files found; run scripts/run_pythia_qq200_signed_vndelta.sh first.")


if __name__ == "__main__":
    main()
