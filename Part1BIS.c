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

int readTemperatureFile(const char* filename, DataPoint** dataPoints) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening temperature file");
        return -1;
    }

    int size = 0;
    int capacity = 10;
    *dataPoints = (DataPoint*)malloc(capacity * sizeof(DataPoint));
    if (*dataPoints == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return -1;
    }

    while (!feof(file)) {
        char timestamp[20];
        double temperature;
        if (fscanf(file, "{\"%19[^\"]\": \"%lf\"}\n", timestamp, &temperature) == 2) {
            strcpy((*dataPoints)[size].timestamp, timestamp);
            (*dataPoints)[size].temperature = temperature;
            size++;
            if (size >= capacity) {
                capacity *= 2;
                *dataPoints = (DataPoint*)realloc(*dataPoints, capacity * sizeof(DataPoint));
                if (*dataPoints == NULL) {
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
    if (file == NULL) {
        perror("Error opening humidity file");
        return -1;
    }

    int size = 0;
    int capacity = 10;
    *humidityPoints = (HumidityPoint*)malloc(capacity * sizeof(HumidityPoint));
    if (*humidityPoints == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return -1;
    }

    while (!feof(file)) {
        char timestamp[20];
        int humidity;
        if (fscanf(file, "{\"%19[^\"]\": \"%d\"}\n", timestamp, &humidity) == 2) {
            strcpy((*humidityPoints)[size].timestamp, timestamp);
            (*humidityPoints)[size].humidity = humidity;
            size++;
            if (size >= capacity) {
                capacity *= 2;
                *humidityPoints = (HumidityPoint*)realloc(*humidityPoints, capacity * sizeof(HumidityPoint));
                if (*humidityPoints == NULL) {
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

int binaryInterpolationSearch(DataPoint* dataPoints, int size, const char* target) {
    int low = 0, high = size - 1;

    while (low <= high && strcmp(target, dataPoints[low].timestamp) >= 0 && strcmp(target, dataPoints[high].timestamp) <= 0) {
        if (strcmp(dataPoints[low].timestamp, dataPoints[high].timestamp) == 0) {
            if (strcmp(dataPoints[low].timestamp, target) == 0) {
                return low;
            } else {
                return -1;
            }
        }

        int pos = low + ((double)(high - low) * (strcmp(target, dataPoints[low].timestamp)) /
                         (strcmp(dataPoints[high].timestamp, dataPoints[low].timestamp)));

        if (strcmp(dataPoints[pos].timestamp, target) == 0) {
            return pos;
        }

        if (strcmp(dataPoints[pos].timestamp, target) < 0) {
            low = pos + 1;
        } else {
            high = pos - 1;
        }
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
    scanf("%s", userTimestamp);

    int tempIndex = binaryInterpolationSearch(temperatureData, tempSize, userTimestamp);
    int humIndex = binaryInterpolationSearch((DataPoint*)humidityData, humSize, userTimestamp);

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