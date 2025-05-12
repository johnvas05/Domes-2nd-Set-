#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef struct{
    char timestamp[20];
    double temperature;
} DataPoint;

int readFile(const char* filename, DataPoint** dataPoints) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
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

    char buffer[2186];
    while (fgets(buffer, sizeof(buffer), file)) {
        char* token = strtok(buffer, ","); // Split the line by commas
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
                        fclose(file);
                        return -1;
                    }
                }
            }
            token = strtok(NULL, ","); // Move to the next token
        }
    }

    fclose(file);
    return size;
}

void writeFile(const char* filename, DataPoint* dataPoints, int size) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        return;
    }

    for (int i = 0; i < size; i++) {
        fprintf(file, "{\"%s\": \"%f\"}\n", dataPoints[i].timestamp, dataPoints[i].temperature);
    }

    fclose(file);
    printf("Sorted contents written to %s\n", filename);
}

int partition(DataPoint arr[], int low, int high) {
    float pivot = arr[high].temperature;
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        if (arr[j].temperature < pivot) {
            i++;
            DataPoint temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
    DataPoint temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;
    return (i + 1);
}

void quickSort(DataPoint arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

int main() {
    const char* filename = "tempm.txt";
    const char* outputFilename = "sorted_tempm_QuickSort.txt";
    DataPoint* dataPoints = NULL;
    int size = readFile(filename, &dataPoints);

    if (size == -1) {
        return 1;
    }

    quickSort(dataPoints, 0, size - 1);

    printf("\nSorted data points:\n");
    for (int i = 0; i < size; i++) {
        printf("Timestamp: %s, Temperature: %f\n", dataPoints[i].timestamp, dataPoints[i].temperature);
    }

    writeFile(outputFilename, dataPoints, size);

    free(dataPoints);
    return 0;
}