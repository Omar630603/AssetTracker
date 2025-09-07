import numpy as np
import matplotlib.pyplot as plt

# Create simple dataset
distance = np.array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
rssi = np.array([-45, -52, -58, -63, -67, -71, -75, -78, -82, -85])

# Create figure with two subplots
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

# Plot 1: Linear scale
ax1.scatter(distance, rssi, color='blue', s=50)
ax1.set_xlabel('Distance (m)')
ax1.set_ylabel('RSSI (dBm)')
ax1.set_title('RSSI vs Distance (Linear Scale)')
ax1.grid(True, alpha=0.3)
ax1.set_ylim(-90, -40)

# Plot 2: Log scale using 10^(RSSI/10)
rssi_log_converted = 10**(rssi/10)  # Convert RSSI using 10^(RSSI/10)
ax2.scatter(distance, rssi_log_converted, color='red', s=50)
ax2.set_xlabel('Distance (m)')
ax2.set_ylabel('10^(RSSI/10)')
ax2.set_title('RSSI vs Distance (Log Scale)')
ax2.set_yscale('log')
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.show()

# Print the dataset
print("Dataset:")
print("Distance (m):", distance)
print("RSSI (dBm):", rssi)