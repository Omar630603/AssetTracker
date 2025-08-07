import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# --- Style setup (print-friendly, visible for presentations/print) ---
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams['figure.facecolor'] = 'white'
plt.rcParams['axes.facecolor'] = 'white'
plt.rcParams.update({'font.size': 21})
COLORS = ['#0080ff', '#e74c3c', '#27ae60']

# --- Load your real data ---
FNAME = 'location_logs_export.csv'
df = pd.read_csv(FNAME, header=None)
df.columns = [
    "id", "asset_id", "tag_id", "rssi", "kalman_rssi", "estimated_distance",
    "type", "status", "reader_name", "created_at", "updated_at"
]
df['created_at'] = pd.to_datetime(df['created_at'])
df['minute'] = (df['created_at'] - df['created_at'].min()).dt.total_seconds() // 60
df['minute'] = df['minute'].astype(int)

# --- Cumulative log count by reader ---
cum_room = df[df['reader_name'] == 'Asset_Reader_01'].groupby('minute')['id'].count().cumsum()
cum_hall = df[df['reader_name'] == 'Asset_Reader_02'].groupby('minute')['id'].count().cumsum()
cum_total = df.groupby('minute')['id'].count().cumsum()

# Fill all minutes for smooth lines
minutes = np.arange(0, df['minute'].max() + 1)
cum_room = cum_room.reindex(minutes, method='ffill').fillna(0)
cum_hall = cum_hall.reindex(minutes, method='ffill').fillna(0)
cum_total = cum_total.reindex(minutes, method='ffill').fillna(0)

# --- PLOT (real data only) ---
plt.figure(figsize=(13, 7))
plt.plot(minutes, cum_room, label='Room Reader (Explicit)', color=COLORS[0], linewidth=4, zorder=2)
plt.plot(minutes, cum_hall, label='Hallway Reader (Pattern)', color=COLORS[1], linewidth=4, zorder=2)
plt.plot(minutes, cum_total, label='Total Logs', color=COLORS[2], linewidth=5, linestyle='--', zorder=1)

plt.xlabel('Time (minutes)', fontsize=20, fontweight='bold')
plt.ylabel('Cumulative Log Count', fontsize=20, fontweight='bold')
plt.title('Cumulative Asset Log Count per Reader (1 Hour)', 
          fontsize=26, fontweight='bold', pad=26)
plt.ylim(0, max(cum_total.max(), 30) * 1.08)
plt.xlim(0, minutes.max())
plt.legend(frameon=True, fancybox=True, shadow=True, fontsize=17, loc='upper left')
plt.grid(alpha=0.3, linestyle='-', linewidth=1)
plt.tight_layout()
plt.show()

# --- Stats summary ---
# Real log counts
room_logs = int(cum_room.max())
hall_logs = int(cum_hall.max())
total_logs = int(cum_total.max())

print("\nLOG COUNTS SUMMARY (Real Data)")
print(f"Room Reader logs:    {room_logs}")
print(f"Hallway Reader logs: {hall_logs}")
print(f"Total logs:          {total_logs}")

# Theoretical log counts (no deduplication, for your system: 1 scan every 30 sec, 5 tags, 1 hour)
scans_per_hour = 60 * 60 // 30
tags_per_reader = 5
room_theoretical = scans_per_hour * tags_per_reader
# Assume hallway reader sees ~3 tags per scan on avg (from your previous simulation), so:
hall_theoretical = int(scans_per_hour * 3)
total_theoretical = room_theoretical + hall_theoretical

print("\nTHEORETICAL LOG COUNTS (No Dedup/Threshold)")
print(f"Room Reader (max):   {room_theoretical}")
print(f"Hallway Reader (max):{hall_theoretical}")
print(f"Total (max):         {total_theoretical}")

# Reduction effectiveness
room_reduction = 100 * (1 - room_logs / room_theoretical)
hall_reduction = 100 * (1 - hall_logs / hall_theoretical)
total_reduction = 100 * (1 - total_logs / total_theoretical)

print("\nThreshold/deduplication effectiveness:")
print(f"  Room reduction:    {room_reduction:.1f}% fewer logs")
print(f"  Hall reduction:    {hall_reduction:.1f}% fewer logs")
print(f"  Overall reduction: {total_reduction:.1f}% fewer logs")
