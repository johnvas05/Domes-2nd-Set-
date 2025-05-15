import numpy as np
from sklearn.linear_model import LinearRegression
import json
from datetime import datetime


def load_temperatures(filename='tempm.txt'):
    """Load temperatures from file and return sorted timestamps and temperatures as lists"""
    timestamps = []
    temperatures = []
    with open(filename, 'r') as file:
        for line in file:
            json_data = json.loads(line.strip())
            for timestamp, temp in json_data.items():
                if temp:  # Skip empty temperature values
                    dt = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S')
                    timestamp = dt.strftime('%Y-%m-%dT%H:%M:%S')
                    timestamps.append(timestamp)
                    temperatures.append(float(temp))
    # Sort data by timestamps
    sorted_data = sorted(zip(timestamps, temperatures))
    timestamps, temperatures = zip(*sorted_data)
    return list(timestamps), list(temperatures)


def load_humidities(filename='hum.txt'):
    """Load humidities from file and return sorted timestamps and humidities as lists"""
    hum_timestamps = []
    humidities = []
    with open(filename, 'r') as file:
        for line in file:
            json_data = json.loads(line.strip())
            for timestamp, hum in json_data.items():
                if hum:  # Skip empty humidity values
                    dt = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S')
                    timestamp = dt.strftime('%Y-%m-%dT%H:%M:%S')
                    hum_timestamps.append(timestamp)
                    humidities.append(float(hum))
    # Sort data by timestamps
    sorted_data = sorted(zip(hum_timestamps, humidities))
    if not sorted_data:
        return [], []
    hum_timestamps, humidities = zip(*sorted_data)
    return list(hum_timestamps), list(humidities)


def timestamp_to_number(timestamp):
    """Convert timestamp string to Unix timestamp (seconds since epoch)"""
    dt = datetime.fromisoformat(timestamp)
    return dt.timestamp()


def normalize_timestamp(ts):
    try:
        dt = datetime.strptime(ts, '%Y-%m-%dT%H:%M:%S')
        return dt.strftime('%Y-%m-%dT%H:%M:%S')
    except Exception:
        return ts  # fallback if format is wrong


def bis(timestamps, temperatures, target_time):
    """Binary Interpolation Search"""
    low, high = 0, len(timestamps) - 1
    steps = 0

    while low <= high:
        steps += 1
        if timestamps[high] == timestamps[low]:
            pos = low
        else:
            pos = (low + high) // 2

        if timestamps[pos] == target_time:
            return temperatures[pos], steps
        elif timestamps[pos] < target_time:
            low = pos + 1
        else:
            high = pos - 1

    return None, steps


def labis(timestamps, temperatures, target_time, model):
    """Learning Augmented Binary Interpolation Search"""
    X = np.array([timestamp_to_number(t) for t in timestamps]).reshape(-1, 1)
    y = np.arange(len(timestamps))

    # Train model
    model.fit(X, y)

    # Predict position based on target timestamp
    target_timestamp = timestamp_to_number(target_time)
    predicted_pos = int(model.predict([[target_timestamp]])[0])
    predicted_pos = max(0, min(predicted_pos, len(timestamps) - 1))

    # Search around predicted position
    steps = 1  # Count the prediction as one step
    low = max(0, predicted_pos - 10)
    high = min(len(timestamps) - 1, predicted_pos + 10)

    while low <= high:
        steps += 1
        mid = (low + high) // 2
        if timestamps[mid] == target_time:
            return temperatures[mid], steps
        elif timestamps[mid] < target_time:
            low = mid + 1
        else:
            high = mid - 1

    return None, steps


def knn_search(timestamps, temperatures, target_time, k=5):
    """Find k nearest neighbors to the target_time."""
    target_num = timestamp_to_number(target_time)
    pairs = [(abs(timestamp_to_number(t) - target_num), t, temp)
             for t, temp in zip(timestamps, temperatures)]
    pairs.sort()
    return [(t, temp) for _, t, temp in pairs[:k]]


def main():
    # Load data
    try:
        timestamps, temperatures = load_temperatures()
        print(f"Temperature data loaded successfully! ({len(timestamps)} records)")
        print("Sample data:", list(zip(timestamps[:5], temperatures[:5])))  # Debugging
    except FileNotFoundError:
        print("Error: tempm.txt not found!")
        return
    except json.JSONDecodeError:
        print("Error: Invalid JSON format in file!")
        return

    # Load humidity data
    try:
        hum_timestamps, humidities = load_humidities()
        hum_data = dict(zip(hum_timestamps, humidities))
        print(f"Loaded {len(hum_data)} humidity records.")
        print("First 5 keys in hum_data:", list(hum_data.keys())[:5])
        print("Sample keys in hum_data:", list(hum_data.keys())[:10])
        print("Sample keys in temp_data:", list(timestamps[:10]))
    except FileNotFoundError:
        print("Error: hum.txt not found!")
        return
    except json.JSONDecodeError:
        print("Error: Invalid JSON format in hum.txt!")
        return

    while True:
        print("\nEnter timestamp (format: YYYY-MM-DDTHH:MM:SS)")
        print("Or type 'exit' to quit")

        timestamp = input("> ").strip()

        if timestamp.lower() == 'exit':
            break

        try:
            # Validate and normalize the timestamp format (must be T-type)
            dt = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S')
            timestamp = dt.strftime('%Y-%m-%dT%H:%M:%S')
            timestamp = normalize_timestamp(timestamp)

            # Debugging: Print the final normalized timestamp
            print(f"Final normalized timestamp: {timestamp}")

            # Search using both algorithms
            temp_bis, steps_bis = bis(timestamps, temperatures, timestamp)
            temp_labis, steps_labis = labis(timestamps, temperatures, timestamp, LinearRegression())

            # Get humidity for the searched timestamp
            hum = hum_data.get(timestamp, "N/A")

            if timestamp in hum_data:
                print(f"DEBUG: {timestamp} found in hum_data with value: {hum_data[timestamp]}")
            else:
                print(f"DEBUG: {timestamp} NOT found in hum_data.")

            print("\nResults:")
            print("-" * 40)

            if temp_bis is not None:
                print(f"Temperature: {temp_bis}°C")
                print(f"Humidity: {hum}")
                print(f"BIS steps: {steps_bis}")
                print(f"LABIS steps: {steps_labis}")
                print(f"LABIS improvement: {steps_bis - steps_labis} steps")
            else:
                print("No temperature data found for this timestamp.")
                print(f"Humidity: {hum}")
                if hum == "N/A":
                    print(f"DEBUG: {timestamp} not found in hum_data.")
                    print("Hum_data keys for this date:")
                    for k in hum_data:
                        if k.startswith(timestamp[:10]):
                            print(k)
                # Suggest nearby timestamps
                for i, t in enumerate(timestamps):
                    if t > timestamp:
                        print("Nearby timestamps:")
                        print(timestamps[max(0, i-2):min(len(timestamps), i+3)])
                        break

            # KNN search
            k = 5  # or any number you want
            neighbors = knn_search(timestamps, temperatures, timestamp, k)
            print(f"\nKNN ({k} nearest):")
            for t, temp in neighbors:
                hum_neighbor = hum_data.get(t, "N/A")
                print(f"{t}: {temp}°C, Humidity: {hum_neighbor}")

        except ValueError:
            print("Invalid timestamp format! Use YYYY-MM-DDTHH:MM:SS")


if __name__ == "__main__":
    main()