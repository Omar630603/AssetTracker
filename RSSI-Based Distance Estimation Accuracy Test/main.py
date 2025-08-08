import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Style and color
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams['figure.facecolor'] = 'white'
plt.rcParams['axes.facecolor'] = 'white'
plt.rcParams.update({
    'font.size': 20,                 # Medium base font
    'axes.labelsize': 23,
    'axes.titlesize': 28,
    'xtick.labelsize': 25,
    'ytick.labelsize': 25,
    'legend.fontsize': 20,
    'legend.title_fontsize': 22,
    'lines.linewidth': 2.6,           # Medium-thick lines
    'lines.markersize': 9             # Medium markers
})

RAW_COLOR = '#0080ff'
RAW_MAX = '#003366'
RAW_MIN = '#80bfff'
KALMAN_COLOR = '#e74c3c'
KALMAN_MAX = '#800000'
KALMAN_MIN = '#ffb3b3'
first_plot_colors = ['#B7950B', '#196F3D', '#3498db', '#e67e22', '#8e44ad', '#e74c3c']

df = pd.read_csv('data.csv')
df['ref_distance'] = df['ref_distance'].astype(float)
df['estimated_distance'] = df['estimated_distance'].astype(float)
df['raw_rssi'] = df['raw_rssi'].astype(float)
df['kalman_rssi'] = df['kalman_rssi'].astype(float)

tags = df['tag'].unique()
ref_distances = sorted(df['ref_distance'].unique())

# 1. Estimated Distance at Each Reference Distance
for ref in ref_distances:
    plt.figure(figsize=(11, 7))
    plt.axhline(ref, color='black', linestyle='--', linewidth=2.2,
                label=f'Distance: {ref} m', alpha=0.8, zorder=1)
    for i, tag in enumerate(tags):
        vals = df[(df['ref_distance'] == ref) & (df['tag'] == tag)]['estimated_distance'].values
        plt.plot(range(1, len(vals)+1), vals,
                 marker='o', markersize=9, linewidth=2.4,
                 color=first_plot_colors[i % len(first_plot_colors)],
                 label=f'{tag}', alpha=0.85, zorder=2,
                 markerfacecolor='white', markeredgewidth=1.5)
    plt.xlabel('Sample Index', fontsize=22, fontweight='bold')
    plt.ylabel('Estimated Distance (m)', fontsize=22, fontweight='bold')
    # plt.title(f'Estimated Distance at Reference = {ref} m',
    #           fontsize=28, fontweight='bold', pad=20)
    plt.ylim(0, max(11, ref+2))
    plt.xlim(1, len(vals))
    plt.legend(frameon=True, fancybox=True, shadow=True, fontsize=23, loc='best', borderpad=1)
    plt.grid(alpha=0.35, linestyle='-', linewidth=1.1)
    plt.tight_layout()
    plt.show()

# 2. RSSI vs Sample Index (Raw & Kalman, each tag separate figure, first trial of each distance)
SAMPLES_PER_TRIAL = 10
for tag in tags:
    plt.figure(figsize=(13, 7))
    tag_data = []
    tag_kalman = []
    for ref in ref_distances:
        sub = df[(df['ref_distance'] == ref) & (df['tag'] == tag)].head(SAMPLES_PER_TRIAL)
        tag_data.extend(sub['raw_rssi'].values)
        tag_kalman.extend(sub['kalman_rssi'].values)
    x = np.arange(1, len(tag_data)+1)
    kalman_smooth = pd.Series(tag_kalman).rolling(window=7, center=True, min_periods=1).mean()
    plt.plot(x, tag_data, color=RAW_COLOR, linewidth=2.6, label='Raw RSSI')
    plt.plot(x, kalman_smooth, color=KALMAN_COLOR, linewidth=3.2, linestyle='--', label='Kalman Filtered RSSI')
    plt.xlabel('Sample Index 10 of Each Distance', fontsize=22, fontweight='bold')
    plt.ylabel('RSSI (dBm)', fontsize=22, fontweight='bold')
    # plt.title(f'RSSI Signal (Raw vs Kalman) - {tag}', fontsize=28, fontweight='bold', pad=20)
    plt.grid(alpha=0.35, linestyle='-', linewidth=1.1)
    plt.legend(loc='upper right', frameon=True, fancybox=True, shadow=True, fontsize=23, borderpad=1)
    plt.tight_layout()
    plt.show()

# 3. Min/Mean/Max RSSI per Reference Distance (Raw & Kalman, vertical lines & average lines)
for tag in tags:
    plt.figure(figsize=(13, 8))
    sub = df[df['tag'] == tag]
    g_raw = sub.groupby('ref_distance')['raw_rssi'].agg(['min', 'mean', 'max'])
    g_kal = sub.groupby('ref_distance')['kalman_rssi'].agg(['min', 'mean', 'max'])
    x = np.array(g_raw.index)
    width = 0.11
    x_raw = x - width/2
    x_kal = x + width/2
    plt.vlines(x_raw, g_raw['min'], g_raw['max'], color=RAW_COLOR, linewidth=6, alpha=0.5)
    plt.vlines(x_kal, g_kal['min'], g_kal['max'], color=KALMAN_COLOR, linewidth=6, alpha=0.5)
    plt.scatter(x_raw, g_raw['min'], color=RAW_MIN, marker='_', s=360, label='Raw Min')
    plt.scatter(x_raw, g_raw['max'], color=RAW_MAX, marker='_', s=360, label='Raw Max')
    plt.scatter(x_kal, g_kal['min'], color=KALMAN_MIN, marker='_', s=360, label='Kalman Min')
    plt.scatter(x_kal, g_kal['max'], color=KALMAN_MAX, marker='_', s=360, label='Kalman Max')
    plt.plot(x_raw, g_raw['mean'], color=RAW_COLOR, marker='o', markersize=12, linewidth=3.2, label='Raw Mean')
    plt.plot(x_kal, g_kal['mean'], color=KALMAN_COLOR, marker='o', markersize=12, linewidth=3.2, linestyle='--', label='Kalman Mean')
    plt.xlabel('Distance (m)', fontsize=22, fontweight='bold')
    plt.ylabel('RSSI (dBm)', fontsize=22, fontweight='bold')
    # plt.title(f'Min/Max/Mean RSSI (Raw & Kalman) by Distance - {tag}', fontsize=28, fontweight='bold', pad=20)
    handles, labels = plt.gca().get_legend_handles_labels()
    by_label = dict(zip(labels, handles))
    plt.legend(by_label.values(), by_label.keys(), frameon=True, fancybox=True, shadow=True, fontsize=23, ncol=2, borderpad=1)
    plt.grid(alpha=0.35, linestyle='-', linewidth=1.1)
    plt.tight_layout()
    plt.show()

# 4. Error Distribution Histogram
df['raw_estimated'] = 10**(( -68.0 - df['raw_rssi'])/(10*2.5))
df['raw_error'] = np.abs(df['raw_estimated'] - df['ref_distance'])
df['kalman_estimated'] = 10**(( -68.0 - df['kalman_rssi'])/(10*2.5))
df['kalman_error'] = np.abs(df['kalman_estimated'] - df['ref_distance'])

plt.figure(figsize=(13, 7))
plt.hist(df['raw_error'], bins=15, alpha=0.7, label='Raw RSSI Error',
         color=RAW_COLOR, edgecolor='white', linewidth=2)
plt.hist(df['kalman_error'], bins=15, alpha=0.7, label='Kalman Filtered Error',
         color=KALMAN_COLOR, edgecolor='white', linewidth=2)
raw_mean = df['raw_error'].mean()
kalman_mean = df['kalman_error'].mean()
plt.axvline(raw_mean, color=RAW_COLOR, linestyle='--', linewidth=3,
           label=f'Raw Mean: {raw_mean:.2f}m')
plt.axvline(kalman_mean, color=KALMAN_COLOR, linestyle='--', linewidth=3,
           label=f'Kalman Mean: {kalman_mean:.2f}m')
plt.xlabel('Absolute Error (meters)', fontsize=22, fontweight='bold')
plt.ylabel('Frequency', fontsize=22, fontweight='bold')
# plt.title('Distance Estimation Error Distribution Comparison',
#           fontsize=28, fontweight='bold', pad=20)
plt.legend(frameon=True, fancybox=True, shadow=True, fontsize=23, borderpad=1)
plt.grid(alpha=0.35, linestyle='-', linewidth=1.1)
plt.tight_layout()
plt.show()

# ---------- Insights Output ----------
df['abs_error'] = np.abs(df['estimated_distance'] - df['ref_distance'])
print("\n" + "="*50)
print("🎯 KEY ACCURACY INSIGHTS")
print("="*50)
for tag, tagdf in df.groupby('tag'):
    print(f"\n📊 {tag} Performance Analysis")
    print("-" * 30)
    summary = tagdf.groupby('ref_distance').agg(
        mean_estimated=('estimated_distance', 'mean'),
        std_estimated=('estimated_distance', 'std'),
        mean_abs_error=('abs_error', 'mean'),
        std_abs_error=('abs_error', 'std'),
        min_error=('abs_error', 'min'),
        max_error=('abs_error', 'max')
    )
    print(summary.round(3))
    print(f"\n📏 Distance-wise Error Summary:")
    for ref in summary.index:
        m = summary.loc[ref, 'mean_abs_error']
        s = summary.loc[ref, 'std_abs_error']
        print(f"  📍 {ref}m: Mean Error = {m:.2f}m (±{s:.2f}m)")
    overall_error = summary['mean_abs_error'].mean()
    print(f"\n🎯 Overall Mean Absolute Error: {overall_error:.2f}m")

mean_raw_err = df['raw_error'].mean()
mean_kalman_err = df['kalman_error'].mean()
improvement = mean_raw_err - mean_kalman_err
improvement_pct = (improvement / mean_raw_err) * 100

print("\n" + "="*50)
print("🔬 KALMAN FILTER PERFORMANCE")
print("="*50)
print(f"📈 Raw RSSI Error:      {mean_raw_err:.2f}m")
print(f"📉 Kalman Filter Error: {mean_kalman_err:.2f}m")
print(f"✅ Improvement:         {improvement:.2f}m ({improvement_pct:.1f}% better)")

if 'cycle' in df.columns:
    min_rssi = df.groupby('cycle')['raw_rssi'].min()
    drift_rate = min_rssi.diff().mean()
    if drift_rate < -0.2:
        print(f"\n⚠️  WARNING: RSSI drifting at {drift_rate:.2f} dBm/cycle")
        print("   Possible causes: battery discharge or tag movement")
    else:
        print(f"\n✅ RSSI stability: Good (drift rate: {drift_rate:.2f} dBm/cycle)")
