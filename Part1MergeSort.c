#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Structure definition
typedef struct {
    char timestamp[20];
    double temperature;
} DataPoint;

// Function declarations
void mergeSort(DataPoint* arr, int left, int right);
void merge(DataPoint* arr, int left, int mid, int right);
void writeFile(const char* filename, DataPoint* dataPoints, int size);
int readFile(const char* filename, DataPoint** dataPoints);

// Merge function implementation
void merge(DataPoint* arr, int left, int mid, int right) {
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    // Create temporary arrays
    DataPoint* L = (DataPoint*)malloc(n1 * sizeof(DataPoint));
    DataPoint* R = (DataPoint*)malloc(n2 * sizeof(DataPoint));

    // Copy data to temporary arrays
    for (i = 0; i < n1; i++)
        L[i] = arr[left + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[mid + 1 + j];

    // Merge the temporary arrays back into arr
    i = 0;
    j = 0;
    k = left;
    while (i < n1 && j < n2) {
        if (L[i].temperature <= R[j].temperature) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // Copy remaining elements of L[] if any
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    // Copy remaining elements of R[] if any
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

    free(L);
    free(R);
}

// MergeSort function implementation
void mergeSort(DataPoint* arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;

        // Sort first and second halves
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);

        // Merge the sorted halves
        merge(arr, left, mid, right);
    }
}

// ReadFile function implementation
int readFile(const char* filename, DataPoint** dataPoints) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    // Get the file size to allocate appropriate buffer
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

    free(buffer);
    fclose(file);
    return size;
}

// WriteFile function implementation
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

// Main function
int main() {
    DataPoint* dataPoints;
    const char* inputFile = "C:\\Users\\mober\\CLionProjects\\Domes 2\\tempm.txt";
    const char* outputFile = "sorted_temperatures_MergeSort.txt";

    int size = readFile(inputFile, &dataPoints);
    if (size < 0) {
        printf("Error reading file\n");
        return 1;
    }

    mergeSort(dataPoints, 0, size - 1);
    writeFile(outputFile, dataPoints, size);

    free(dataPoints);
    return 0;
}