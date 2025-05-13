#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char timestamp[20];
    double temperature;
} DataPoint;

// Helper function to parse a single JSON object from a line
int parseLine(const char* line, DataPoint** dataPoints, int* size, int* capacity) {
    char* lineCopy = strdup(line);  // Make a copy of the line to tokenize
    char* token = strtok(lineCopy, "{},");
    
    while (token != NULL) {
        char timestamp[30];
        char temperatureStr[30];
        
        // Skip leading whitespace
        while (isspace(*token)) token++;
        
        if (sscanf(token, "\"%[^\"]\": \"%[^\"]\"", timestamp, temperatureStr) == 2) {
            // Reallocate if needed
            if (*size >= *capacity) {
                *capacity *= 2;
                DataPoint* temp = (DataPoint*)realloc(*dataPoints, *capacity * sizeof(DataPoint));
                if (temp == NULL) {
                    free(lineCopy);
                    return -1;
                }
                *dataPoints = temp;
            }
            
            // Store the data
            strcpy((*dataPoints)[*size].timestamp, timestamp);
            (*dataPoints)[*size].temperature = atof(temperatureStr);
            (*size)++;
        }
        
        token = strtok(NULL, "{},");
    }
    
    free(lineCopy);
    return 0;
}

int readFile(const char* filename, DataPoint** dataPoints) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        perror("Error details");
        return -1;
    }

    int size = 0;
    int capacity = 100;  // Start with larger capacity
    *dataPoints = (DataPoint*)malloc(capacity * sizeof(DataPoint));
    if (*dataPoints == NULL) {
        fclose(file);
        return -1;
    }

    char line[4096];  // Buffer for reading lines (adjust size if needed)

    while (fgets(line, sizeof(line), file) != NULL) {
        // Remove newline if present
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        if (parseLine(line, dataPoints, &size, &capacity) < 0) {
            fclose(file);
            return -1;
        }
    }

    fclose(file);

    // Sort the array by timestamp
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
        printf("First entry: {%s: %f}\n", 
               (*dataPoints)[0].timestamp, (*dataPoints)[0].temperature);
        printf("Last entry: {%s: %f}\n", 
               (*dataPoints)[size-1].timestamp, (*dataPoints)[size-1].temperature);
    }

    return size;
}

// Binary Interpolation Search implementation
int binaryInterpolationSearch(DataPoint* arr, int size, const char* target_timestamp) {
    int left = 0;
    int right = size - 1;
    
    while (left <= right) {
        // Compare the timestamps directly
        int cmp = strcmp(arr[left].timestamp, target_timestamp);
        if (cmp == 0) return left;
        
        cmp = strcmp(arr[right].timestamp, target_timestamp);
        if (cmp == 0) return right;
        
        // If target is outside the range
        if (strcmp(target_timestamp, arr[left].timestamp) < 0 ||
            strcmp(target_timestamp, arr[right].timestamp) > 0) {
            return -1;
        }
        
        // Interpolate position
        int pos = left + ((right - left) / 2);
        
        cmp = strcmp(arr[pos].timestamp, target_timestamp);
        if (cmp == 0) return pos;
        
        if (cmp < 0) {
            left = pos + 1;
        } else {
            right = pos - 1;
        }
    }
    
    return -1;
}

int main() {
    DataPoint* dataPoints;
    const char* inputFile = "C:\\Users\\mober\\CLionProjects\\Domes 2\\tempm.txt";
    
    printf("Starting to read file: %s\n", inputFile);
    int size = readFile(inputFile, &dataPoints);
    
    if (size <= 0) {
        printf("Error reading file or no data found\n");
        return 1;
    }
    
    printf("\nSuccessfully read %d entries\n", size);

    char search_timestamp[20];
    printf("\nEnter timestamp to search (format: YYYY-MM-DDTHH:MM:SS): ");
    scanf("%19s", search_timestamp);
    
    int result = binaryInterpolationSearch(dataPoints, size, search_timestamp);
    
    if (result != -1) {
        printf("Found timestamp at index %d with temperature %f\n", 
               result, dataPoints[result].temperature);
    } else {
        printf("Timestamp not found\n");
        // Print some entries around where it should be
        for (int i = 0; i < size; i++) {
            if (strcmp(dataPoints[i].timestamp, search_timestamp) > 0) {
                int start = (i - 2 >= 0) ? i - 2 : 0;
                int end = (i + 2 < size) ? i + 2 : size - 1;
                printf("\nNearby entries:\n");
                for (int j = start; j <= end; j++) {
                    printf("%d: {%s: %f}\n", j, dataPoints[j].timestamp, dataPoints[j].temperature);
                }
                break;
            }
        }
    }

    free(dataPoints);
    return 0;
}