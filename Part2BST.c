#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char timestamp[20];
    double temperature;
} DataPoint;

typedef struct {
    char* date;        // Dynamically allocated string for the date
    double totalTemp;
    int count;
    double avgTemp;
} DailyAverage;

int readFile(DataPoint** dataPoints) {
    // Open the file directly with the full path
    FILE* file = fopen("C:/Users/pbili/CLionProjects/Domes-2nd-Set-/tempm.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    // Calculate file size for appropriate buffer allocation
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    // Allocate buffer dynamically
    char* buffer = (char*)malloc((fileSize + 1) * sizeof(char));
    if (buffer == NULL) {
        perror("Buffer allocation failed");
        fclose(file);
        return -1;
    }

    int size = 0;
    int capacity = 10;
    *dataPoints = (DataPoint*)malloc(capacity * sizeof(DataPoint));
    if (*dataPoints == NULL) {
        perror("Memory allocation failed");
        free(buffer);
        fclose(file);
        return -1;
    }

    while (fgets(buffer, fileSize + 1, file)) {
        char* token = strtok(buffer, ",");
        while (token != NULL) {
            char timestamp[20];
            char temperatureStr[10];

            if (sscanf(token, " { \"%[^\"]\" : \"%[^\"]\" }", timestamp, temperatureStr) == 2 ||
                sscanf(token, " \"%[^\"]\" : \"%[^\"]\"", timestamp, temperatureStr) == 2) {
                strcpy((*dataPoints)[size].timestamp, timestamp);
                (*dataPoints)[size].temperature = atof(temperatureStr);
                size++;
                if (size >= capacity) {
                    capacity *= 2;
                    *dataPoints = (DataPoint*)realloc(*dataPoints, capacity * sizeof(DataPoint));
                    if (*dataPoints == NULL) {
                        perror("Memory reallocation failed");
                        free(buffer);
                        fclose(file);
                        return -1;
                    }
                }
            }
            token = strtok(NULL, ",");
        }
    }

    // Free the dynamically allocated buffer
    free(buffer);
    fclose(file);
    return size;
}

// Extracts the date from a timestamp
char* extractDate(const char* timestamp) {
    // Assuming timestamps are in the format "YYYY-MM-DD HH:MM:SS"
    char* date = (char*)malloc(11 * sizeof(char)); // 10 for "YYYY-MM-DD" + 1 for '\0'
    if (date == NULL) {
        perror("Failed to allocate memory for date");
        return NULL;
    }

    strncpy(date, timestamp, 10);
    date[10] = '\0';

    return date;
}

// Frees the memory used by the daily averages array
void freeDailyAverages(DailyAverage* dailyAvgs, int size) {
    if (dailyAvgs == NULL) return;

    for (int i = 0; i < size; i++) {
        free(dailyAvgs[i].date);
    }
    free(dailyAvgs);
}

// Calculates the average temperature for each day
int calculateDailyAverages(DataPoint* dataPoints, int size, DailyAverage** dailyAvgs) {
    if (size <= 0) return 0;

    int dailyCount = 0;
    int dailyCapacity = 10;
    *dailyAvgs = (DailyAverage*)malloc(dailyCapacity * sizeof(DailyAverage));
    if (*dailyAvgs == NULL) {
        perror("Failed to allocate memory for daily averages");
        return -1;
    }

    for (int i = 0; i < size; i++) {
        char* currentDate = extractDate(dataPoints[i].timestamp);
        if (currentDate == NULL) {
            freeDailyAverages(*dailyAvgs, dailyCount);
            return -1;
        }

        // Check if the date already exists in the array
        int found = 0;
        for (int j = 0; j < dailyCount; j++) {
            if (strcmp((*dailyAvgs)[j].date, currentDate) == 0) {
                (*dailyAvgs)[j].totalTemp += dataPoints[i].temperature;
                (*dailyAvgs)[j].count++;
                found = 1;
                free(currentDate); // We no longer need this copy
                break;
            }
        }

        // If not found, add a new date entry
        if (!found) {
            if (dailyCount >= dailyCapacity) {
                dailyCapacity *= 2;
                *dailyAvgs = (DailyAverage*)realloc(*dailyAvgs, dailyCapacity * sizeof(DailyAverage));
                if (*dailyAvgs == NULL) {
                    perror("Memory reallocation failed");
                    free(currentDate);
                    freeDailyAverages(*dailyAvgs, dailyCount);
                    return -1;
                }
            }

            (*dailyAvgs)[dailyCount].date = currentDate;
            (*dailyAvgs)[dailyCount].totalTemp = dataPoints[i].temperature;
            (*dailyAvgs)[dailyCount].count = 1;
            dailyCount++;
        }
    }

    // Calculate the averages
    for (int i = 0; i < dailyCount; i++) {
        (*dailyAvgs)[i].avgTemp = (*dailyAvgs)[i].totalTemp / (*dailyAvgs)[i].count;
    }

    return dailyCount;
}

void writeDailyAverages(DailyAverage* dailyAvgs, int size) {
    // Open the output file directly with its name
    FILE* file = fopen("daily_average_temperatures.txt", "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        return;
    }

    fprintf(file, "Date,Average Temperature,Number of Measurements\n");
    for (int i = 0; i < size; i++) {
        fprintf(file, "%s,%.2f,%d\n", dailyAvgs[i].date, dailyAvgs[i].avgTemp, dailyAvgs[i].count);
    }

    fclose(file);
    printf("Daily average temperatures saved to daily_average_temperatures.txt\n");
}

int main() {
    DataPoint* dataPoints = NULL;
    DailyAverage* dailyAverages = NULL;

    // Read data from the file
    int dataSize = readFile(&dataPoints);
    if (dataSize == -1) {
        return 1;
    }

    printf("Read %d data points\n", dataSize);

    // Calculate the daily averages
    int daysCount = calculateDailyAverages(dataPoints, dataSize, &dailyAverages);
    if (daysCount == -1) {
        free(dataPoints);
        return 1;
    }

    printf("\nDaily Average Temperatures:\n");
    printf("---------------------------\n");
    for (int i = 0; i < daysCount; i++) {
        printf("Date: %s, Average Temperature: %.2f°C, Number of measurements: %d\n",
               dailyAverages[i].date, dailyAverages[i].avgTemp, dailyAverages[i].count);
    }

    // Save the results
    writeDailyAverages(dailyAverages, daysCount);

    // Free memory
    free(dataPoints);
    freeDailyAverages(dailyAverages, daysCount);

    return 0;
}