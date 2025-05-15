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
                    timestamps.append(timestamp)
                    temperatures.append(float(temp))
    # Sort data by timestamps
    sorted_data = sorted(zip(timestamps, temperatures))
    timestamps, temperatures = zip(*sorted_data)
    return list(timestamps), list(temperatures)


def timestamp_to_number(timestamp):
    """Convert timestamp string to Unix timestamp (seconds since epoch)"""
    dt = datetime.fromisoformat(timestamp)
    return dt.timestamp()


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

    while True:
        print("\nEnter timestamp (format: YYYY-MM-DDTHH:MM:SS)")
        print("Or type 'exit' to quit")

        timestamp = input("> ").strip()

        if timestamp.lower() == 'exit':
            break

        try:
            # Validate and normalize the timestamp format (must be T-type)
            dt = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S')
            timestamp = dt.strftime('%Y-%m-%dT%H:%M:%S')  # Ensure canonical format

            # Debugging: Print the final normalized timestamp
            print(f"Final normalized timestamp: {timestamp}")

            # Search using both algorithms
            temp_bis, steps_bis = bis(timestamps, temperatures, timestamp)
            temp_labis, steps_labis = labis(timestamps, temperatures, timestamp, LinearRegression())

            print("\nResults:")
            print("-" * 40)

            if temp_bis is not None:
                print(f"Temperature: {temp_bis}°C")
                print(f"BIS steps: {steps_bis}")
                print(f"LABIS steps: {steps_labis}")
                print(f"LABIS improvement: {steps_bis - steps_labis} steps")
            else:
                print("No temperature data found for this timestamp.")
                # Suggest nearby timestamps
                for i, t in enumerate(timestamps):
                    if t > timestamp:
                        print("Nearby timestamps:")
                        print(timestamps[max(0, i-2):min(len(timestamps), i+3)])
                        break

        except ValueError:
            print("Invalid timestamp format! Use YYYY-MM-DDTHH:MM:SS")


if __name__ == "__main__":
    main()