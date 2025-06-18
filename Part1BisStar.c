#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char timestamp[20];
    double temperature;
    double humidity;
} DataPoint;

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

// Helper function to parse a single JSON object from a line
int parseLine(const char* line, DataPoint** dataPoints, int* size, int* capacity, int isTemperature) {
    char* lineCopy = strdup(line);
    char* token = strtok(lineCopy, "{},");

    while (token != NULL) {
        char timestamp[30];
        char valueStr[30];

        while (isspace(*token)) token++;

        if (sscanf(token, "\"%[^\"]\": \"%[^\"]\"", timestamp, valueStr) == 2) {
            int found = -1;
            for (int i = 0; i < *size; i++) {
                if (strcmp((*dataPoints)[i].timestamp, timestamp) == 0) {
                    found = i;
                    break;
                }
            }

            if (found == -1) {
                if (*size >= *capacity) {
                    *capacity *= 2;
                    DataPoint* temp = (DataPoint*)realloc(*dataPoints, *capacity * sizeof(DataPoint));
                    if (temp == NULL) {
                        free(lineCopy);
                        return -1;
                    }
                    *dataPoints = temp;
                }

                strcpy((*dataPoints)[*size].timestamp, timestamp);
                if (isTemperature) {
                    (*dataPoints)[*size].temperature = atof(valueStr);
                    (*dataPoints)[*size].humidity = -1;
                } else {
                    (*dataPoints)[*size].humidity = atof(valueStr);
                    (*dataPoints)[*size].temperature = -1;
                }
                (*size)++;
            } else {
                if (isTemperature) {
                    (*dataPoints)[found].temperature = atof(valueStr);
                } else {
                    (*dataPoints)[found].humidity = atof(valueStr);
                }
            }
        }

        token = strtok(NULL, "{},");
    }

    free(lineCopy);
    return 0;
}

int readFiles(const char* tempFile, const char* humFile, DataPoint** dataPoints) {
    int size = 0;
    int capacity = 100;
    *dataPoints = (DataPoint*)malloc(capacity * sizeof(DataPoint));
    if (*dataPoints == NULL) {
        return -1;
    }

    FILE* file = fopen(tempFile, "r");
    if (file == NULL) {
        printf("Error opening temperature file: %s\n", tempFile);
        perror("Error details");
        free(*dataPoints);
        return -1;
    }

    char line[4096];
    while (fgets(line, sizeof(line), file) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        if (parseLine(line, dataPoints, &size, &capacity, 1) < 0) {
            fclose(file);
            free(*dataPoints);
            return -1;
        }
    }
    fclose(file);

    file = fopen(humFile, "r");
    if (file == NULL) {
        printf("Error opening humidity file: %s\n", humFile);
        perror("Error details");
        free(*dataPoints);
        return -1;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        if (parseLine(line, dataPoints, &size, &capacity, 0) < 0) {
            fclose(file);
            free(*dataPoints);
            return -1;
        }
    }
    fclose(file);

    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (strcmp((*dataPoints)[j].timestamp, (*dataPoints)[j + 1].timestamp) > 0) {
                DataPoint temp = (*dataPoints)[j];
                (*dataPoints)[j] = (*dataPoints)[j + 1];
                (*dataPoints)[j + 1] = temp;
            }
        }
    }

    printf("\nTotal entries read: %d\n", size);
    if (size > 0) {
        printf("First entry: {%s: temp=%.2f, hum=%.2f}\n",
               (*dataPoints)[0].timestamp, (*dataPoints)[0].temperature, (*dataPoints)[0].humidity);
        printf("Last entry: {%s: temp=%.2f, hum=%.2f}\n",
               (*dataPoints)[size-1].timestamp, (*dataPoints)[size-1].temperature, (*dataPoints)[size-1].humidity);
    }

    return size;
}

// Binary Interpolation Search with BIS variation (jump step *= 2)
int bisVariationSearch(DataPoint* arr, int size, const char* target_timestamp) {
    long long targetKey = timestampToKey(target_timestamp);
    int left = 0, right = size - 1;

    while (left <= right) {
        long long leftKey = timestampToKey(arr[left].timestamp);
        long long rightKey = timestampToKey(arr[right].timestamp);

        if (targetKey < leftKey || targetKey > rightKey) {
            return -1;
        }

        if (leftKey == rightKey) {
            return (targetKey == leftKey) ? left : -1;
        }

        int pos = left + (int)(((double)(right - left) * (targetKey - leftKey)) / (rightKey - leftKey));

        if (pos < left) pos = left;
        if (pos > right) pos = right;

        long long posKey = timestampToKey(arr[pos].timestamp);

        if (posKey == targetKey) {
            return pos;
        }

        // Exponential jump phase
        int step = 1;
        if (posKey < targetKey) {
            int i = pos + 1;
            while (i <= right && timestampToKey(arr[i].timestamp) < targetKey) {
                i = i + step;
                step *= 2;
            }
            left = (i - step / 2 < pos + 1) ? pos + 1 : i - step / 2;
            right = (i <= right) ? i : right;
        } else {
            int i = pos - 1;
            while (i >= left && timestampToKey(arr[i].timestamp) > targetKey) {
                i = i - step;
                step *= 2;
            }
            right = (i + step / 2 > pos - 1) ? pos - 1 : i + step / 2;
            left = (i >= left) ? i : left;
        }

        // If the search window is small, switch to binary search
        if (right - left < 5) {
            while (left <= right) {
                int mid = left + (right - left) / 2;
                long long midKey = timestampToKey(arr[mid].timestamp);
                if (midKey == targetKey) return mid;
                if (midKey < targetKey) {
                    left = mid + 1;
                } else {
                    right = mid - 1;
                }
            }
            return -1;
        }
    }

    return -1;
}

int main() {
    DataPoint* dataPoints;
    const char* tempFile = "C:\\Users\\mober\\CLionProjects\\Domes 2\\tempm.txt";
    const char* humFile = "C:\\Users\\mober\\CLionProjects\\Domes 2\\hum.txt";

    printf("Starting to read files...\n");
    int size = readFiles(tempFile, humFile, &dataPoints);

    if (size <= 0) {
        printf("Error reading files or no data found\n");
        return 1;
    }

    printf("\nSuccessfully read %d entries\n", size);

    char search_timestamp[20];
    printf("\nEnter timestamp to search (format: YYYY-MM-DDTHH:MM:SS): ");
    scanf("%19s", search_timestamp);

    int result = bisVariationSearch(dataPoints, size, search_timestamp);

    if (result != -1) {
        printf("Found timestamp at index %d:\n", result);
        printf("Temperature: %.1f\n", dataPoints[result].temperature);
        printf("Humidity: %.1f\n", dataPoints[result].humidity);
    } else {
        printf("Timestamp not found\n");
        for (int i = 0; i < size; i++) {
            if (strcmp(dataPoints[i].timestamp, search_timestamp) > 0) {
                int start = (i - 2 >= 0) ? i - 2 : 0;
                int end = (i + 2 < size) ? i + 2 : size - 1;
                printf("\nNearby entries:\n");
                for (int j = start; j <= end; j++) {
                    printf("%d: {%s: temp=%.2f, hum=%.2f}\n",
                           j, dataPoints[j].timestamp, dataPoints[j].temperature, dataPoints[j].humidity);
                }
                break;
            }
        }
    }

    free(dataPoints);
    return 0;
}
