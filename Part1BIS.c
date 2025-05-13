#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char timestamp[20];
    double temperature;
} DataPoint;

typedef struct {
    char timestamp[20];
    int humidity;
} HumidityPoint;

// Converts timestamps to numeric keys for interpolation
long long timestampToKey(const char* timestamp) {
    int year, month, day, hour, minute, second;
    sscanf(timestamp, "%4d-%2d-%2dT%2d:%2d:%2d",
           &year, &month, &day, &hour, &minute, &second);
    return (long long)year * 10000000000LL +
           (long long)month * 100000000LL +
           (long long)day * 1000000LL +
           (long long)hour * 10000LL +
           (long long)minute * 100LL +
           (long long)second;
}

int readTemperatureFile(const char* filename, DataPoint** dataPoints) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening temperature file");
        return -1;
    }

    int size = 0, capacity = 10;
    *dataPoints = malloc(capacity * sizeof(DataPoint));
    if (!*dataPoints) {
        perror("Memory allocation failed");
        fclose(file);
        return -1;
    }

    char line[100];
    while (fgets(line, sizeof(line), file)) {
        char timestamp[20];
        double temperature;
        if (sscanf(line, "{\"%19[^\"]\": \"%lf\"}", timestamp, &temperature) == 2) {
            strcpy((*dataPoints)[size].timestamp, timestamp);
            (*dataPoints)[size].temperature = temperature;
            size++;
            if (size >= capacity) {
                capacity *= 2;
                *dataPoints = realloc(*dataPoints, capacity * sizeof(DataPoint));
                if (!*dataPoints) {
                    perror("Memory reallocation failed");
                    fclose(file);
                    return -1;
                }
            }
        }
    }

    fclose(file);
    return size;
}

int readHumidityFile(const char* filename, HumidityPoint** humidityPoints) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening humidity file");
        return -1;
    }

    int size = 0, capacity = 10;
    *humidityPoints = malloc(capacity * sizeof(HumidityPoint));
    if (!*humidityPoints) {
        perror("Memory allocation failed");
        fclose(file);
        return -1;
    }

    char line[100];
    while (fgets(line, sizeof(line), file)) {
        char timestamp[20];
        int humidity;
        if (sscanf(line, "{\"%19[^\"]\": \"%d\"}", timestamp, &humidity) == 2) {
            strcpy((*humidityPoints)[size].timestamp, timestamp);
            (*humidityPoints)[size].humidity = humidity;
            size++;
            if (size >= capacity) {
                capacity *= 2;
                *humidityPoints = realloc(*humidityPoints, capacity * sizeof(HumidityPoint));
                if (!*humidityPoints) {
                    perror("Memory reallocation failed");
                    fclose(file);
                    return -1;
                }
            }
        }
    }

    fclose(file);
    return size;
}

// Binary Interpolation Search using timestamp-to-key mapping
int binaryInterpolationSearch(const char timestamps[][20], int size, const char* target) {
    long long targetKey = timestampToKey(target);
    int low = 0, high = size - 1;

    while (low <= high) {
        long long lowKey = timestampToKey(timestamps[low]);
        long long highKey = timestampToKey(timestamps[high]);

        if (targetKey < lowKey || targetKey > highKey)
            return -1;

        if (lowKey == highKey) {
            return (targetKey == lowKey) ? low : -1;
        }

        int pos = low + (int)(((double)(high - low) * (targetKey - lowKey)) / (highKey - lowKey));

        if (pos < low || pos > high) return -1;

        long long posKey = timestampToKey(timestamps[pos]);

        if (posKey == targetKey)
            return pos;
        else if (posKey < targetKey)
            low = pos + 1;
        else
            high = pos - 1;
    }

    return -1;
}

int main() {
    const char* temperatureFile = "c:\\Users\\thodo\\Documents\\dome II\\Domes-2nd-Set-\\tempm.txt";
    const char* humidityFile = "c:\\Users\\thodo\\Documents\\dome II\\Domes-2nd-Set-\\hum.txt";

    DataPoint* temperatureData = NULL;
    HumidityPoint* humidityData = NULL;

    int tempSize = readTemperatureFile(temperatureFile, &temperatureData);
    int humSize = readHumidityFile(humidityFile, &humidityData);

    if (tempSize == -1 || humSize == -1) {
        return 1;
    }

    char userTimestamp[20];
    printf("Enter a timestamp (YYYY-MM-DDTHH:MM:SS): ");
    scanf("%19s", userTimestamp);

    // Extract just the timestamps for searching
    char tempTimestamps[tempSize][20];
    char humTimestamps[humSize][20];
    for (int i = 0; i < tempSize; i++) strcpy(tempTimestamps[i], temperatureData[i].timestamp);
    for (int i = 0; i < humSize; i++) strcpy(humTimestamps[i], humidityData[i].timestamp);

    int tempIndex = binaryInterpolationSearch(tempTimestamps, tempSize, userTimestamp);
    int humIndex = binaryInterpolationSearch(humTimestamps, humSize, userTimestamp);

    if (tempIndex != -1 && humIndex != -1) {
        printf("Timestamp: %s\n", userTimestamp);
        printf("Temperature: %.2f\n", temperatureData[tempIndex].temperature);
        printf("Humidity: %d\n", humidityData[humIndex].humidity);
    } else {
        printf("Timestamp not found in the data.\n");
    }

    free(temperatureData);
    free(humidityData);
    return 0;
}
