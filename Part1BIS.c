#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    char timestamp[20];
    double temperature;
    int humidity;
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

// Reads temperature and humidity data into a single DataPoint array
int readFiles(const char* tempFile, const char* humFile, DataPoint** dataPoints) {
    FILE* tempFp = fopen(tempFile, "r");
    FILE* humFp = fopen(humFile, "r");
    if (!tempFp || !humFp) {
        perror("Error opening files");
        if (tempFp) fclose(tempFp);
        if (humFp) fclose(humFp);
        return -1;
    }

    int size = 0, capacity = 10;
    *dataPoints = malloc(capacity * sizeof(DataPoint));
    if (!*dataPoints) {
        perror("Memory allocation failed");
        fclose(tempFp);
        fclose(humFp);
        return -1;
    }

    char line[100];
    while (fgets(line, sizeof(line), tempFp)) {
        char timestamp[20];
        double temperature;
        if (sscanf(line, "{\"%19[^\"]\": \"%lf\"}", timestamp, &temperature) == 2) {
            if (size >= capacity) {
                capacity *= 2;
                *dataPoints = realloc(*dataPoints, capacity * sizeof(DataPoint));
                if (!*dataPoints) {
                    perror("Memory reallocation failed");
                    fclose(tempFp);
                    fclose(humFp);
                    return -1;
                }
            }
            strcpy((*dataPoints)[size].timestamp, timestamp);
            (*dataPoints)[size].temperature = temperature;
            (*dataPoints)[size].humidity = -1; // Placeholder for humidity
            size++;
        }
    }
    fclose(tempFp);

    while (fgets(line, sizeof(line), humFp)) {
        char timestamp[20];
        int humidity;
        if (sscanf(line, "{\"%19[^\"]\": \"%d\"}", timestamp, &humidity) == 2) {
            for (int i = 0; i < size; i++) {
                if (strcmp((*dataPoints)[i].timestamp, timestamp) == 0) {
                    (*dataPoints)[i].humidity = humidity;
                    break;
                }
            }
        }
    }
    fclose(humFp);

    return size;
}

// Συνάρτηση μετατροπής timestamp σε αριθμητικό κλειδί (υπάρχει ήδη στο αρχείο σου)
long long timestampToKey(const char* timestamp);

// Jump Interpolation Search για DataPoint
int jump_interpolation_search(DataPoint* data, int n, const char* targetTimestamp) {
    long long key = timestampToKey(targetTimestamp);
    int left = 0, right = n - 1;
    int size = right - left + 1;
    int next = left + (int)((size * (double)(key - timestampToKey(data[left].timestamp))) /
                            (timestampToKey(data[right].timestamp) - timestampToKey(data[left].timestamp)));

    while (left <= right && key != timestampToKey(data[next].timestamp)) {
        int i = 0;
        size = right - left + 1;
        if (size <= 3) { // Απευθείας αναζήτηση
            for (int j = left; j <= right; j++) {
                if (timestampToKey(data[j].timestamp) == key) return j;
            }
            return -1;
        }
        if (key > timestampToKey(data[next].timestamp)) {
            while (next + (i + 1) * (int)sqrt(size) <= right &&
                   key > timestampToKey(data[next + (i + 1) * (int)sqrt(size)].timestamp)) {
                i++;
            }
            left = next + i * (int)sqrt(size);
            right = next + (i + 1) * (int)sqrt(size);
            if (right > n - 1) right = n - 1;
        } else {
            while (next - (i + 1) * (int)sqrt(size) >= left &&
                   key < timestampToKey(data[next - (i + 1) * (int)sqrt(size)].timestamp)) {
                i++;
            }
            right = next - i * (int)sqrt(size);
            left = next - (i + 1) * (int)sqrt(size);
            if (left < 0) left = 0;
        }
        size = right - left + 1;
        long long leftKey = timestampToKey(data[left].timestamp);
        long long rightKey = timestampToKey(data[right].timestamp);
        if (rightKey == leftKey) break; // αποφυγή διαίρεσης με το μηδέν
        next = left + (int)((size * (double)(key - leftKey)) / (rightKey - leftKey));
    }
    if (timestampToKey(data[next].timestamp) == key) return next;
    return -1;
}

// Swap function for DataPoint
void swap(DataPoint* a, DataPoint* b) {
    DataPoint temp = *a;
    *a = *b;
    *b = temp;
}

// Partition function for timestamp
int partition(DataPoint arr[], int low, int high) {
    long long pivot = timestampToKey(arr[high].timestamp);
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        if (timestampToKey(arr[j].timestamp) < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

// QuickSort for DataPoint by timestamp
void quickSortByTimestamp(DataPoint arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSortByTimestamp(arr, low, pi - 1);
        quickSortByTimestamp(arr, pi + 1, high);
    }
}

int main() {
    const char* temperatureFile = "c:\\Users\\teo\\Documents\\DOMES PART II\\Domes-2nd-Set-\\tempm.txt";
    const char* humidityFile = "c:\\Users\\teo\\Documents\\DOMES PART II\\Domes-2nd-Set-\\hum.txt";

    DataPoint* dataPoints = NULL;
    int dataSize = readFiles(temperatureFile, humidityFile, &dataPoints);

    if (dataSize == -1) {
        return 1;
    }

    printf("\nTotal entries read: %d\n", dataSize);
    if (dataSize > 0) {
        printf("First entry: {%s: temp=%.2f, hum=%d}\n",
               dataPoints[0].timestamp, dataPoints[0].temperature, dataPoints[0].humidity);
        printf("Last entry: {%s: temp=%.2f, hum=%d}\n",
               dataPoints[dataSize - 1].timestamp, dataPoints[dataSize - 1].temperature, dataPoints[dataSize - 1].humidity);
    }

    char userTimestamp[20];
    printf("\nEnter a timestamp to search (YYYY-MM-DDTHH:MM:SS): ");
    scanf("%19s", userTimestamp);
    
    quickSortByTimestamp(dataPoints, 0, dataSize - 1);

    int index = jump_interpolation_search(dataPoints, dataSize, userTimestamp);

    if (index != -1) {
        printf("\nFound timestamp at index %d:\n", index);
        printf("Timestamp: %s\n", dataPoints[index].timestamp);
        printf("Temperature: %.2f\n", dataPoints[index].temperature);
        printf("Humidity: %d\n", dataPoints[index].humidity);
    } else {
        printf("\nTimestamp not found.\n");

        // Print nearby entries for context
        for (int i = 0; i < dataSize; i++) {
            if (strcmp(dataPoints[i].timestamp, userTimestamp) > 0) {
                int start = (i - 2 >= 0) ? i - 2 : 0;
                int end = (i + 2 < dataSize) ? i + 2 : dataSize - 1;
                printf("\nNearby entries:\n");
                for (int j = start; j <= end; j++) {
                    printf("%d: {%s: temp=%.2f, hum=%d}\n",
                           j, dataPoints[j].timestamp, dataPoints[j].temperature, dataPoints[j].humidity);
                }
                break;
            }
        }
    }

    free(dataPoints);
    return 0;
}
