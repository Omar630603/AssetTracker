import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from scipy.stats import linregress

def load_and_process_data(file_path, experiment_type):
    df = pd.read_csv(file_path)
    
    df_exp = df[df['Experiment Type'] == experiment_type]
    df_success = df_exp[df_exp['Status'] == 'Success']
    
    stats = df_success.groupby('Distance (meters)').agg({
        'RSSI': ['min', 'mean', 'max', 'count'],
        'Calculated Distance': 'mean'
    }).round(2)
    
    failed_counts = df_exp[df_exp['Status'] == 'Failed'].groupby('Distance (meters)').size()
    
    summary = pd.DataFrame({
        'Min RSSI': stats['RSSI']['min'],
        'Avg RSSI': stats['RSSI']['mean'],
        'Max RSSI': stats['RSSI']['max'],
        'Avg Calc Distance': stats['Calculated Distance']['mean'],
        'Success Count': stats['RSSI']['count'],
        'Failed Count': failed_counts
    }).fillna(0)
    
    return summary, df_exp

def plot_rssi_stats(summary, title, df_exp):
    # Error bar plot
    plt.figure(figsize=(10, 6))
    plt.errorbar(summary.index, summary['Avg RSSI'],
                yerr=[summary['Avg RSSI'] - summary['Min RSSI'],
                      summary['Max RSSI'] - summary['Avg RSSI']],
                fmt='o-', capsize=5, label='RSSI Reading', color='teal', markersize=8)
    
    plt.xticks(np.arange(min(summary.index), max(summary.index) + 1, 1))
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.xlabel('Reference Distance (meters)')
    plt.ylabel('RSSI (dBm)')
    plt.title(f'{title} - Min/Average/Max RSSI Values')
    plt.legend()
    plt.show()
    
    # Scatter plot with markers for success and failed attempts
    plt.figure(figsize=(10, 6))
    success_points = df_exp[df_exp['Status'] == 'Success']
    failed_points = df_exp[df_exp['Status'] == 'Failed']
    
    plt.scatter(success_points['Distance (meters)'], success_points['RSSI'], 
                color='green', label='Success', alpha=0.7)
    plt.scatter(failed_points['Distance (meters)'], failed_points['RSSI'], 
                color='red', marker='x', label='Failed', alpha=0.8)
    
    for dist in summary.index:
        success_count = int(summary.loc[dist, 'Success Count'])
        failed_count = int(summary.loc[dist, 'Failed Count'])
        plt.annotate(f'S:{success_count}\nF:{failed_count}', 
                    xy=(dist, summary.loc[dist, 'Avg RSSI']),
                    xytext=(0, 50), textcoords='offset points',
                    ha='left', va='top',
                    bbox=dict(facecolor='white', edgecolor='none', alpha=0.7))
    
    plt.xticks(np.arange(min(summary.index), max(summary.index) + 1, 1))
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.xlabel('Reference Distance (meters)')
    plt.ylabel('RSSI (dBm)')
    plt.title(f'{title} - Success/Failed RSSI Markers')
    plt.legend()
    plt.show()
    
    # Plot for regression line on success points
    plt.figure(figsize=(10, 6))
    
    slope, intercept, _, _, _ = linregress(success_points['Distance (meters)'], success_points['RSSI'])
    
    reg_line_x = np.linspace(min(success_points['Distance (meters)']), max(success_points['Distance (meters)']), 100)
    reg_line_y = slope * reg_line_x + intercept
    
    plt.plot(reg_line_x, reg_line_y, color='blue', label='Regression Line', linewidth=2)
    
    plt.scatter(success_points['Distance (meters)'], success_points['RSSI'], 
                color='green', label='Success', alpha=0.7)
    
    plt.xticks(np.arange(min(summary.index), max(summary.index) + 1, 1))
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.xlabel('Reference Distance (meters)')
    plt.ylabel('RSSI (dBm)')
    plt.title(f'{title} - Regression Line for Success RSSI')
    plt.legend()
    plt.show()

def plot_moving_experiment(file_path):
    df = pd.read_csv(file_path)
    df_success = df[df['Status'] == 'Success']
    df_failed = df[df['Status'] == 'Failed']
    
    stats = df_success.groupby('Reading Number').agg({
        'RSSI': ['min', 'mean', 'max'],
        'Calculated Distance': 'mean'
    })
    
    plt.figure(figsize=(10, 6))
    plt.plot(stats.index, stats['RSSI']['mean'], 'o-', color='darkblue', label='Average RSSI')
    plt.scatter(df_failed['Reading Number'], df_failed['RSSI'], color='red', label='Failed', marker='x')
    
    plt.xticks(np.arange(min(df.index), max(df.index) + 1, 1))
    
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.xlabel('Time (Reading Number)')
    plt.ylabel('RSSI (dBm)')
    plt.title('Moving Experiment - RSSI Over Time')
    plt.legend()
    plt.show()

# Load and plot Clear Path experiment
clear_summary, clear_data = load_and_process_data('./.output/clear_path_experiment.csv', 'Clear')
plot_rssi_stats(clear_summary, 'Clear Path', clear_data)

# Print Clear Path Summary
print("\nClear Path Summary:")
print(clear_summary)

# Load and plot Wall Path experiment
wall_summary, wall_data = load_and_process_data('./.output/wall_experiment.csv', 'Wall')
plot_rssi_stats(wall_summary, 'Wall Path', wall_data)

# Print Wall Path Summary
print("\nWall Path Summary:")
print(wall_summary)

# Plot Moving Experiment
plot_moving_experiment('./.output/moving_experiment.csv')