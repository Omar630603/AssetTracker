import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import make_interp_spline

# Set style for better looking plots
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams['figure.facecolor'] = 'white'
plt.rcParams['axes.facecolor'] = 'white'

# Define color palette
colors = ['#2E8B57', '#FF6B35', '#4A90E2', '#F39C12', '#8E44AD', '#E74C3C']
accent_color = '#34495E'

# -- Load & Prepare Data --
df = pd.read_csv('data.csv')
df['ref_distance'] = df['ref_distance'].astype(float)
df['estimated_distance'] = df['estimated_distance'].astype(float)
df['raw_rssi'] = df['raw_rssi'].astype(float)
df['kalman_rssi'] = df['kalman_rssi'].astype(float)

tags = df['tag'].unique()
ref_distances = sorted(df['ref_distance'].unique())

# ----- 1. Estimated Distance per Reference Distance -----
for ref in ref_distances:
    plt.figure(figsize=(10, 6))
    plt.axhline(ref, color='#95A5A6', linestyle='--', linewidth=2, 
                label=f'Reference: {ref}m', alpha=0.8, zorder=1)
    
    for i, tag in enumerate(tags):
        vals = df[(df['ref_distance'] == ref) & (df['tag'] == tag)]['estimated_distance'].values
        plt.plot(range(1, len(vals)+1), vals, 
                marker='o', markersize=8, linewidth=2.5, 
                color=colors[i % len(colors)], 
                label=f'{tag}', alpha=0.9, zorder=2,
                markerfacecolor='white', markeredgewidth=2)
    
    plt.xlabel('Measurement Number', fontsize=12, fontweight='bold')
    plt.ylabel('Estimated Distance (m)', fontsize=12, fontweight='bold')
    plt.title(f'Distance Estimation Accuracy at {ref}m Reference', 
              fontsize=14, fontweight='bold', pad=20)
    plt.ylim(0, 11)
    plt.xlim(1, 5)
    plt.legend(frameon=True, fancybox=True, shadow=True, fontsize=10)
    plt.grid(alpha=0.3, linestyle='-', linewidth=0.5)
    plt.tight_layout()
    plt.show()

# ----- 2. RSSI vs Reference Distance (Mean, Smooth, Per Device) -----
for i, tag in enumerate(tags):
    plt.figure(figsize=(10, 6))
    subset = df[df['tag'] == tag]
    means_raw = subset.groupby('ref_distance')['raw_rssi'].mean()
    means_kalman = subset.groupby('ref_distance')['kalman_rssi'].mean()
    
    # Smooth lines with enhanced colors
    xnew = np.linspace(min(means_raw.index), max(means_raw.index), 200)
    smooth_raw = make_interp_spline(means_raw.index, means_raw.values)(xnew)
    smooth_kalman = make_interp_spline(means_kalman.index, means_kalman.values)(xnew)
    
    plt.plot(xnew, smooth_raw, color='#E74C3C', linewidth=3, 
             label='Raw RSSI', alpha=0.8)
    plt.plot(xnew, smooth_kalman, color='#3498DB', linewidth=3, 
             linestyle='--', label='Kalman Filtered RSSI', alpha=0.8)
    
    plt.scatter(means_raw.index, means_raw.values, color='#E74C3C', 
                s=80, edgecolors='white', linewidth=2, zorder=5)
    plt.scatter(means_kalman.index, means_kalman.values, color='#3498DB', 
                s=80, edgecolors='white', linewidth=2, zorder=5)
    
    plt.xlabel('Reference Distance (m)', fontsize=12, fontweight='bold')
    plt.ylabel('Mean RSSI (dBm)', fontsize=12, fontweight='bold')
    plt.title(f'RSSI Signal Strength vs Distance - {tag}', 
              fontsize=14, fontweight='bold', pad=20)
    plt.grid(alpha=0.3, linestyle='-', linewidth=0.5)
    plt.legend(frameon=True, fancybox=True, shadow=True, fontsize=11)
    plt.tight_layout()
    plt.show()

# ----- 3. Min/Mean/Max RSSI per Reference Distance (Enhanced Cap Plot) -----
for tag in tags:
    for col, name, base_color in [
        ('raw_rssi', 'Raw RSSI', '#E74C3C'),
        ('kalman_rssi', 'Kalman Filtered RSSI', '#3498DB')
    ]:
        plt.figure(figsize=(10, 6))
        sub = df[df['tag'] == tag].groupby('ref_distance')[col].agg(['min', 'mean', 'max'])
        
        for idx, row in sub.iterrows():
            # Vertical line (min to max) with gradient effect
            plt.plot([idx, idx], [row['min'], row['max']], 
                    color=base_color, linewidth=4, alpha=0.7, zorder=1)
            
            # Enhanced horizontal "caps" with colors
            capwidth = 0.08
            # Min cap (red)
            plt.plot([idx - capwidth, idx + capwidth], [row['min'], row['min']], 
                    color='#E67E22', linewidth=4, alpha=0.9, zorder=2)
            # Max cap (green)
            plt.plot([idx - capwidth, idx + capwidth], [row['max'], row['max']], 
                    color='#27AE60', linewidth=4, alpha=0.9, zorder=2)
        
        # Mean line with enhanced styling
        plt.plot(sub.index, sub['mean'], marker='o', color='#8E44AD', 
                linewidth=3, markersize=10, label='Mean', 
                markerfacecolor='white', markeredgewidth=2, zorder=3)
        
        # Add custom legend for caps
        from matplotlib.lines import Line2D
        legend_elements = [
            Line2D([0], [0], color='#8E44AD', linewidth=3, marker='o', 
                   markersize=8, label='Mean', markerfacecolor='white', markeredgewidth=2),
            Line2D([0], [0], color='#27AE60', linewidth=4, label='Maximum'),
            Line2D([0], [0], color='#E67E22', linewidth=4, label='Minimum'),
            Line2D([0], [0], color=base_color, linewidth=4, alpha=0.7, label='Range')
        ]
        
        plt.xlabel('Reference Distance (m)', fontsize=12, fontweight='bold')
        plt.ylabel('RSSI (dBm)', fontsize=12, fontweight='bold')
        plt.title(f'{name} Variability Analysis - {tag}', 
                  fontsize=14, fontweight='bold', pad=20)
        plt.grid(alpha=0.3, linestyle='-', linewidth=0.5)
        plt.legend(handles=legend_elements, frameon=True, fancybox=True, 
                  shadow=True, fontsize=10)
        plt.tight_layout()
        plt.show()

# ----- 4. Error Distribution Histogram (Enhanced) -----
df['raw_estimated'] = 10**(( -68.0 - df['raw_rssi'])/(10*2.5))
df['raw_error'] = np.abs(df['raw_estimated'] - df['ref_distance'])
df['kalman_estimated'] = 10**(( -68.0 - df['kalman_rssi'])/(10*2.5))
df['kalman_error'] = np.abs(df['kalman_estimated'] - df['ref_distance'])

plt.figure(figsize=(12, 6))

# Create side-by-side histograms with better styling
plt.hist(df['raw_error'], bins=15, alpha=0.7, label='Raw RSSI Error', 
         color='#E74C3C', edgecolor='white', linewidth=1.5)
plt.hist(df['kalman_error'], bins=15, alpha=0.7, label='Kalman Filtered Error', 
         color='#3498DB', edgecolor='white', linewidth=1.5)

# Add mean lines
raw_mean = df['raw_error'].mean()
kalman_mean = df['kalman_error'].mean()
plt.axvline(raw_mean, color='#C0392B', linestyle='--', linewidth=2, 
           label=f'Raw Mean: {raw_mean:.2f}m')
plt.axvline(kalman_mean, color='#2980B9', linestyle='--', linewidth=2, 
           label=f'Kalman Mean: {kalman_mean:.2f}m')

plt.xlabel('Absolute Error (meters)', fontsize=12, fontweight='bold')
plt.ylabel('Frequency', fontsize=12, fontweight='bold')
plt.title('Distance Estimation Error Distribution Comparison', 
          fontsize=14, fontweight='bold', pad=20)
plt.legend(frameon=True, fancybox=True, shadow=True, fontsize=11)
plt.grid(alpha=0.3, linestyle='-', linewidth=0.5)
plt.tight_layout()
plt.show()

# ---------- Enhanced Insights Output ----------
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

# Enhanced battery drift warning
min_rssi = df.groupby('cycle')['raw_rssi'].min()
drift_rate = min_rssi.diff().mean()
if drift_rate < -0.2:
    print(f"\n⚠️  WARNING: RSSI drifting at {drift_rate:.2f} dBm/cycle")
    print("   Possible causes: battery discharge or tag movement")
else:
    print(f"\n✅ RSSI stability: Good (drift rate: {drift_rate:.2f} dBm/cycle)")