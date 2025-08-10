# export_for_figs.py
# Exports data + style so Octave can rebuild identical-looking figures and save .fig files.

import os, numpy as np, pandas as pd
import matplotlib.pyplot as plt
from scipy.stats import linregress
from scipy.io import savemat

EXPORT_DIR = "./.output/fig_exports"
os.makedirs(EXPORT_DIR, exist_ok=True)

# ---------- STYLE (match your original code) ----------
# Matplotlib named colors used in your original code, encoded as RGB in [0..1]
STYLE = {
    "fig_size_px": (1000, 600),     # approx 10x6 inches on typical screens
    "grid_linestyle": "--",
    "grid_alpha": 0.7,              # Octave can't do grid alpha; kept for completeness
    "xtick_step": 1,

    "errorbar": {
        "fmt": "o-",
        "capsize": 5,
        "marker_size": 8,
        "color_rgb": (0/255, 128/255, 128/255),   # 'teal'
        "line_width": 1.5,                        # close to MPL default
        "label": "RSSI Reading",
    },
    "scatter_success": {
        "marker": "o",
        "color_rgb": (0/255, 128/255, 0/255),     # 'green'
        "alpha": 0.7,
        "label": "Success",
        "size_pts2": 36,                          # ~MPL default area ~36
    },
    "scatter_failed": {
        "marker": "x",
        "color_rgb": (1.0, 0.0, 0.0),             # 'red'
        "alpha": 0.8,
        "label": "Failed",
        "size_pts2": 36,
    },
    "regression": {
        "color_rgb": (0.0, 0.0, 1.0),             # 'blue'
        "line_width": 2.0,
        "label": "Regression Line",
    },
    "moving_avg": {
        "fmt": "o-",
        "color_rgb": (0/255, 0/255, 139/255),     # 'darkblue'
        "line_width": 1.5,
        "label": "Average RSSI",
    },
    "annotation": {
        # In MPL you used xytext=(0,50) in *points*. Octave text() works in data units.
        # We’ll simulate a similar offset in DATA units using a small fraction of the y-range.
        "y_offset_frac": 0.05,
        "bg_rgb": (1.0, 1.0, 1.0),
        "bg_alpha": 0.7,   # Octave doesn't support alpha on text background; kept for completeness
        "va": "top",
        "ha": "left",
    },
    "labels": {
        "xlabel_rssi": "Reference Distance (meters)",
        "ylabel_rssi": "RSSI (dBm)",
        "xlabel_moving": "Time (Reading Number)",
        "ylabel_moving": "RSSI (dBm)",
    }
}
# -----------------------------------------------------


def load_and_process_data(file_path, experiment_type):
    df = pd.read_csv(file_path)
    df_exp = df[df['Experiment Type'] == experiment_type]
    df_success = df_exp[df_exp['Status'] == 'Success']

    stats = df_success.groupby('Distance (meters)').agg({
        'RSSI': ['min', 'mean', 'max', 'count'],
        'Calculated Distance': 'mean'
    }).round(2)

    failed_counts = df_exp[df_exp['Status'] == 'Failed'] \
                        .groupby('Distance (meters)').size()

    summary = pd.DataFrame({
        'Min RSSI': stats['RSSI']['min'],
        'Avg RSSI': stats['RSSI']['mean'],
        'Max RSSI': stats['RSSI']['max'],
        'Avg Calc Distance': stats['Calculated Distance']['mean'],
        'Success Count': stats['RSSI']['count'],
        'Failed Count': failed_counts
    }).fillna(0)

    summary.index = summary.index.astype(float)
    return summary, df_exp


def export_errorbar_mat(summary, title, base):
    x = summary.index.values.astype(float)
    avg = summary['Avg RSSI'].values.astype(float)
    ymin = summary['Min RSSI'].values.astype(float)
    ymax = summary['Max RSSI'].values.astype(float)
    yerr_lower = avg - ymin
    yerr_upper = ymax - avg

    xticks_vec = np.arange(np.nanmin(x), np.nanmax(x) + STYLE["xtick_step"], STYLE["xtick_step"], dtype=float)

    savemat(os.path.join(EXPORT_DIR, f"{base}_errorbar.mat"), {
        # data
        'x': x, 'avg': avg,
        'yerr_lower': yerr_lower, 'yerr_upper': yerr_upper,
        # labels / titles
        'xlabel': STYLE["labels"]["xlabel_rssi"],
        'ylabel': STYLE["labels"]["ylabel_rssi"],
        'title': f'{title} - Min/Average/Max RSSI Values',
        'legend_rssi': STYLE["errorbar"]["label"],
        # style
        'fig_w': float(STYLE["fig_size_px"][0]), 'fig_h': float(STYLE["fig_size_px"][1]),
        'grid_linestyle': STYLE["grid_linestyle"],
        'color_err_r': float(STYLE["errorbar"]["color_rgb"][0]),
        'color_err_g': float(STYLE["errorbar"]["color_rgb"][1]),
        'color_err_b': float(STYLE["errorbar"]["color_rgb"][2]),
        'err_capsize': float(STYLE["errorbar"]["capsize"]),
        'err_marker_size': float(STYLE["errorbar"]["marker_size"]),
        'err_line_width': float(STYLE["errorbar"]["line_width"]),
        'xticks_vec': xticks_vec,
    })


def export_scatter_mat(df_exp, summary, title, base):
    s = df_exp[df_exp['Status'] == 'Success']
    f = df_exp[df_exp['Status'] == 'Failed']

    dist_vals = summary.index.values.astype(float)
    avg_at_dist = summary['Avg RSSI'].values.astype(float)

    # y offset for text annotations (simulate ~ "xytext=(0,50)" in data units)
    if avg_at_dist.size:
        y_off = (np.nanmax(avg_at_dist) - np.nanmin(avg_at_dist)) * STYLE["annotation"]["y_offset_frac"]
    else:
        y_off = 0.0

    xticks_vec = np.arange(np.nanmin(dist_vals) if dist_vals.size else 0,
                           (np.nanmax(dist_vals) if dist_vals.size else 0) + STYLE["xtick_step"],
                           STYLE["xtick_step"], dtype=float)

    savemat(os.path.join(EXPORT_DIR, f"{base}_scatter.mat"), {
        # data
        'sx': s['Distance (meters)'].values.astype(float),
        'sy': s['RSSI'].values.astype(float),
        'fx': f['Distance (meters)'].values.astype(float),
        'fy': f['RSSI'].values.astype(float),
        'dist_vals': dist_vals,
        'avg_at_dist': avg_at_dist,
        'succ_cnt': summary['Success Count'].values.astype(int),
        'fail_cnt': summary['Failed Count'].values.astype(int),
        'y_off': float(y_off),
        # labels / titles
        'xlabel': STYLE["labels"]["xlabel_rssi"],
        'ylabel': STYLE["labels"]["ylabel_rssi"],
        'title': f'{title} - Success/Failed RSSI Markers',
        'legend_success': STYLE["scatter_success"]["label"],
        'legend_failed': STYLE["scatter_failed"]["label"],
        # style
        'fig_w': float(STYLE["fig_size_px"][0]), 'fig_h': float(STYLE["fig_size_px"][1]),
        'grid_linestyle': STYLE["grid_linestyle"],
        'sc_s_r': float(STYLE["scatter_success"]["color_rgb"][0]),
        'sc_s_g': float(STYLE["scatter_success"]["color_rgb"][1]),
        'sc_s_b': float(STYLE["scatter_success"]["color_rgb"][2]),
        'sc_s_alpha': float(STYLE["scatter_success"]["alpha"]),
        'sc_s_size': float(STYLE["scatter_success"]["size_pts2"]),
        'sc_s_marker': 'o',
        'sc_f_r': float(STYLE["scatter_failed"]["color_rgb"][0]),
        'sc_f_g': float(STYLE["scatter_failed"]["color_rgb"][1]),
        'sc_f_b': float(STYLE["scatter_failed"]["color_rgb"][2]),
        'sc_f_alpha': float(STYLE["scatter_failed"]["alpha"]),
        'sc_f_size': float(STYLE["scatter_failed"]["size_pts2"]),
        'sc_f_marker': 'x',
        'xticks_vec': xticks_vec,
        'text_bg_r': float(STYLE["annotation"]["bg_rgb"][0]),
        'text_bg_g': float(STYLE["annotation"]["bg_rgb"][1]),
        'text_bg_b': float(STYLE["annotation"]["bg_rgb"][2]),
    })


def export_regression_mat(df_exp, title, base):
    s = df_exp[df_exp['Status'] == 'Success']
    sx = s['Distance (meters)'].values.astype(float)
    sy = s['RSSI'].values.astype(float)
    slope, intercept = (np.nan, np.nan)
    if sx.size >= 2:
        slope, intercept, *_ = linregress(sx, sy)

    xticks_vec = np.arange(np.nanmin(sx) if sx.size else 0,
                           (np.nanmax(sx) if sx.size else 0) + STYLE["xtick_step"],
                           STYLE["xtick_step"], dtype=float)

    savemat(os.path.join(EXPORT_DIR, f"{base}_regression.mat"), {
        # data
        'sx': sx, 'sy': sy,
        'slope': float(slope), 'intercept': float(intercept),
        # labels / titles
        'xlabel': STYLE["labels"]["xlabel_rssi"],
        'ylabel': STYLE["labels"]["ylabel_rssi"],
        'title': f'{title} - Regression Line for Success RSSI',
        # style
        'fig_w': float(STYLE["fig_size_px"][0]), 'fig_h': float(STYLE["fig_size_px"][1]),
        'grid_linestyle': STYLE["grid_linestyle"],
        'reg_r': float(STYLE["regression"]["color_rgb"][0]),
        'reg_g': float(STYLE["regression"]["color_rgb"][1]),
        'reg_b': float(STYLE["regression"]["color_rgb"][2]),
        'reg_lw': float(STYLE["regression"]["line_width"]),
        'sc_s_r': float(STYLE["scatter_success"]["color_rgb"][0]),
        'sc_s_g': float(STYLE["scatter_success"]["color_rgb"][1]),
        'sc_s_b': float(STYLE["scatter_success"]["color_rgb"][2]),
        'sc_s_alpha': float(STYLE["scatter_success"]["alpha"]),
        'sc_s_size': float(STYLE["scatter_success"]["size_pts2"]),
        'sc_s_marker': 'o',
        'xticks_vec': xticks_vec,
    })


def plot_rssi_stats(summary, title, df_exp, base):
    # ------- Python visuals (unchanged look) -------
    # Error bar
    plt.figure(figsize=(10,6))
    plt.errorbar(summary.index, summary['Avg RSSI'],
                 yerr=[summary['Avg RSSI']-summary['Min RSSI'],
                       summary['Max RSSI']-summary['Avg RSSI']],
                 fmt=STYLE["errorbar"]["fmt"],
                 capsize=STYLE["errorbar"]["capsize"],
                 label=STYLE["errorbar"]["label"],
                 color='teal',
                 markersize=STYLE["errorbar"]["marker_size"])
    plt.xticks(np.arange(min(summary.index), max(summary.index) + 1, 1))
    plt.grid(True, linestyle=STYLE["grid_linestyle"], alpha=STYLE["grid_alpha"])
    plt.xlabel(STYLE["labels"]["xlabel_rssi"]); plt.ylabel(STYLE["labels"]["ylabel_rssi"])
    plt.title(f'{title} - Min/Average/Max RSSI Values'); plt.legend(); plt.tight_layout(); plt.show()

    # Scatter S/F with annotations
    s = df_exp[df_exp['Status'] == 'Success']
    f = df_exp[df_exp['Status'] == 'Failed']
    plt.figure(figsize=(10,6))
    plt.scatter(s['Distance (meters)'], s['RSSI'], color='green', label='Success', alpha=0.7)
    plt.scatter(f['Distance (meters)'], f['RSSI'], color='red',   label='Failed',  alpha=0.8, marker='x')
    for dist in summary.index:
        plt.annotate(f"S:{int(summary.loc[dist,'Success Count'])}\nF:{int(summary.loc[dist,'Failed Count'])}",
                     xy=(dist, summary.loc[dist,'Avg RSSI']), xytext=(0,50),
                     textcoords='offset points', ha='left', va='top',
                     bbox=dict(facecolor='white', edgecolor='none', alpha=0.7))
    plt.xticks(np.arange(min(summary.index), max(summary.index) + 1, 1))
    plt.grid(True, linestyle=STYLE["grid_linestyle"], alpha=STYLE["grid_alpha"])
    plt.xlabel(STYLE["labels"]["xlabel_rssi"]); plt.ylabel(STYLE["labels"]["ylabel_rssi"])
    plt.title(f'{title} - Success/Failed RSSI Markers'); plt.legend(); plt.tight_layout(); plt.show()

    # Regression
    plt.figure(figsize=(10,6))
    if len(s) >= 2:
        slope, intercept, *_ = linregress(s['Distance (meters)'], s['RSSI'])
        rx = np.linspace(s['Distance (meters)'].min(), s['Distance (meters)'].max(), 100)
        plt.plot(rx, slope*rx+intercept, color='blue', label='Regression Line', linewidth=2)
    plt.scatter(s['Distance (meters)'], s['RSSI'], color='green', label='Success', alpha=0.7)
    plt.xticks(np.arange(min(summary.index), max(summary.index) + 1, 1))
    plt.grid(True, linestyle=STYLE["grid_linestyle"], alpha=STYLE["grid_alpha"])
    plt.xlabel(STYLE["labels"]["xlabel_rssi"]); plt.ylabel(STYLE["labels"]["ylabel_rssi"])
    plt.title(f'{title} - Regression Line for Success RSSI'); plt.legend(); plt.tight_layout(); plt.show()

    # ------- Export for Octave/MATLAB -------
    export_errorbar_mat(summary, title, base)
    export_scatter_mat(df_exp, summary, title, base)
    export_regression_mat(df_exp, title, base)


def plot_moving_experiment(file_path, base='moving'):
    df = pd.read_csv(file_path)
    df_success = df[df['Status'] == 'Success']
    df_failed  = df[df['Status'] == 'Failed']
    stats = df_success.groupby('Reading Number').agg({'RSSI':['mean']})

    # Python plot (original look)
    plt.figure(figsize=(10,6))
    plt.plot(stats.index, stats['RSSI']['mean'], STYLE["moving_avg"]["fmt"],
             color=(0/255,0/255,139/255), label=STYLE["moving_avg"]["label"])
    if not df_failed.empty:
        plt.scatter(df_failed['Reading Number'], df_failed['RSSI'], color='red', label='Failed', marker='x')
    plt.xticks(np.arange(int(df['Reading Number'].min()), int(df['Reading Number'].max()) + 1, 1))
    plt.grid(True, linestyle=STYLE["grid_linestyle"], alpha=STYLE["grid_alpha"])
    plt.xlabel(STYLE["labels"]["xlabel_moving"]); plt.ylabel(STYLE["labels"]["ylabel_moving"])
    plt.title('Moving Experiment - RSSI Over Time'); plt.legend(); plt.tight_layout(); plt.show()

    # Export for Octave/MATLAB (with style)
    rx = stats.index.values.astype(float)
    xticks_vec = np.arange(np.nanmin(rx) if rx.size else 0,
                           (np.nanmax(rx) if rx.size else 0) + STYLE["xtick_step"],
                           STYLE["xtick_step"], dtype=float)

    savemat(os.path.join(EXPORT_DIR, f'{base}_moving.mat'), {
        'rx': rx,
        'avg': stats['RSSI']['mean'].values.astype(float),
        'fx': df_failed['Reading Number'].values.astype(float),
        'fy': df_failed['RSSI'].values.astype(float),
        # labels / titles
        'xlabel': STYLE["labels"]["xlabel_moving"],
        'ylabel': STYLE["labels"]["ylabel_moving"],
        'title': 'Moving Experiment - RSSI Over Time',
        'legend_avg': STYLE["moving_avg"]["label"],
        'legend_failed': STYLE["scatter_failed"]["label"],
        # style
        'fig_w': float(STYLE["fig_size_px"][0]), 'fig_h': float(STYLE["fig_size_px"][1]),
        'grid_linestyle': STYLE["grid_linestyle"],
        'mv_r': float(STYLE["moving_avg"]["color_rgb"][0]),
        'mv_g': float(STYLE["moving_avg"]["color_rgb"][1]),
        'mv_b': float(STYLE["moving_avg"]["color_rgb"][2]),
        'mv_lw': float(STYLE["moving_avg"]["line_width"]),
        'xticks_vec': xticks_vec,
        'fail_r': float(STYLE["scatter_failed"]["color_rgb"][0]),
        'fail_g': float(STYLE["scatter_failed"]["color_rgb"][1]),
        'fail_b': float(STYLE["scatter_failed"]["color_rgb"][2]),
        'fail_size': float(STYLE["scatter_failed"]["size_pts2"]),
        'fail_alpha': float(STYLE["scatter_failed"]["alpha"]),
    })


# ---- Run like your original flow ----
clear_summary, clear_data = load_and_process_data('./.output/clear_path_experiment.csv', 'Clear')
plot_rssi_stats(clear_summary, 'Clear Path', clear_data, base='clear')

wall_summary, wall_data = load_and_process_data('./.output/wall_experiment.csv', 'Wall')
plot_rssi_stats(wall_summary, 'Wall Path', wall_data, base='wall')

plot_moving_experiment('./.output/moving_experiment.csv', base='moving')
print(f"\nMAT bundles exported to: {os.path.abspath(EXPORT_DIR)}")
