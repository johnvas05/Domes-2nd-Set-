#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef struct{
    char timestamp[20];
    float temperature;
} DataPoint;

int readFile(const char* filename, int** arr) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    int size = 0;
    int capacity = 10;
    *arr = (int*)malloc(capacity * sizeof(int));
    if (*arr == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return -1;
    }

    while (fscanf(file, "%d", &(*arr)[size]) == 1) {
        size++;
        if (size >= capacity) {
            capacity *= 2;
            *arr = (int*)realloc(*arr, capacity * sizeof(int));
            if (*arr == NULL) {
                perror("Memory reallocation failed");
                fclose(file);
                return -1;
            }
        }
    }

    fclose(file);
    return size;
}

void writeFile(const char* filename, int* arr, int size) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        return;
    }

    for (int i = 0; i < size; i++) {
        fprintf(file, "%d ", arr[i]);
    }

    fclose(file);
    printf("Sorted contents written to %s\n", filename);
}

int partition(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            int temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
    int temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;
    return (i + 1);
}

void quickSort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

int main() {
    const char* filename = "tempm.txt";
    int* values = NULL;
    int size = readFile(filename, (int**)&values);

    if (size == -1) {
        return 1; // Exit if file reading failed
    }

    printf("Extracted values:\n");
    for (int i = 0; i < size; i++) {
        printf("%d ", values[i]);
    }
    printf("\n");

    free(values); // Free allocated memory
    return 0;
}