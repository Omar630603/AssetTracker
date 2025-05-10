import csv
import re
import os

def parse_data(file_path):
    experiments = {
        "clear": [],
        "wall": [],
        "moving": []
    }
    current_experiment = "clear"
    current_distance = None
    readings = []
    is_failed = False
    moving_reading_number = 1 

    with open(file_path, 'r') as file:
        for line in file:
            line = line.strip()
            
            if line == "Wall":
                if current_distance is not None and readings:
                    experiments[current_experiment].append((current_distance, readings))
                current_experiment = "wall"
                current_distance = None  
                readings = []
                continue

            elif line.startswith("From 15 meters"):
                if current_distance is not None and readings:
                    experiments[current_experiment].append((current_distance, readings))
                current_experiment = "moving"
                current_distance = None  
                readings = []
                moving_reading_number = 1  
                continue
            
            distance_match = re.match(r"(\d+(\.\d+)?) meters", line)
            if distance_match:
                if current_distance is not None and readings:
                    experiments[current_experiment].append((current_distance, readings))
                current_distance = float(distance_match.group(1))
                readings = []
                is_failed = False
                continue

            reading_match = re.match(r"Reading \d+: RSSI = (-?\d+) \| Calculated Distance = ([\d\.]+) meters", line)
            if reading_match:
                rssi = int(reading_match.group(1))
                calculated_distance = float(reading_match.group(2))
                if current_experiment == "moving":
                    experiments["moving"].append((moving_reading_number, rssi, calculated_distance, "Success"))
                    moving_reading_number += 1
                else:
                    readings.append((rssi, calculated_distance, "Success"))
                is_failed = False 
                
            elif line == "Scanning...":
                if is_failed:
                    if current_experiment == "moving":
                        experiments["moving"].append((moving_reading_number, 0, 0, "Failed"))
                        moving_reading_number += 1
                    else:
                        readings.append((0, 0, "Failed"))
                is_failed = True
        
        if current_distance is not None and readings:
            experiments[current_experiment].append((current_distance, readings))
    
    return experiments

def save_to_csv(experiments):
    output_dir = ".output"
    os.makedirs(output_dir, exist_ok=True)

    filenames = {
        "clear": f"{output_dir}/clear_path_experiment.csv",
        "wall": f"{output_dir}/wall_experiment.csv",
        "moving": f"{output_dir}/moving_experiment.csv"
    }
    
    for exp_type, data in experiments.items():
        with open(filenames[exp_type], 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            if exp_type == "moving":
                writer.writerow(["Experiment Type", "Reading Number", "RSSI", "Calculated Distance", "Status"])
                for reading_num, rssi, calc_dist, status in data:
                    writer.writerow(["Moving", reading_num, rssi, calc_dist, status])
            else:
                writer.writerow(["Distance (meters)", "Reading Number", "RSSI", "Calculated Distance", "Status", "Experiment Type"])
                for distance, readings in data:
                    for i, (rssi, calc_dist, status) in enumerate(readings, start=1):
                        writer.writerow([distance, i, rssi, calc_dist, status, exp_type.capitalize()])

def main():
    file_path = "./data/test.txt"
    
    experiments = parse_data(file_path)
    
    save_to_csv(experiments)
    print("Data has been successfully exported to CSV files in the .output/ folder.")

main()