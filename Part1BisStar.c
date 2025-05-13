#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char timestamp[20];
    double temperature;
    double humidity;    // Added humidity field
} DataPoint;

// Helper function to parse a single JSON object from a line
int parseLine(const char* line, DataPoint** dataPoints, int* size, int* capacity, int isTemperature) {
    char* lineCopy = strdup(line);  // Make a copy of the line to tokenize
    char* token = strtok(lineCopy, "{},");
    
    while (token != NULL) {
        char timestamp[30];
        char valueStr[30];
        
        // Skip leading whitespace
        while (isspace(*token)) token++;
        
        if (sscanf(token, "\"%[^\"]\": \"%[^\"]\"", timestamp, valueStr) == 2) {
            // Check if this timestamp already exists in our array
            int found = -1;
            for (int i = 0; i < *size; i++) {
                if (strcmp((*dataPoints)[i].timestamp, timestamp) == 0) {
                    found = i;
                    break;
                }
            }
            
            if (found == -1) {  // New timestamp
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
                if (isTemperature) {
                    (*dataPoints)[*size].temperature = atof(valueStr);
                    (*dataPoints)[*size].humidity = -1;  // Mark as not set
                } else {
                    (*dataPoints)[*size].humidity = atof(valueStr);
                    (*dataPoints)[*size].temperature = -1;  // Mark as not set
                }
                (*size)++;
            } else {  // Existing timestamp
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
    int capacity = 100;  // Start with larger capacity
    *dataPoints = (DataPoint*)malloc(capacity * sizeof(DataPoint));
    if (*dataPoints == NULL) {
        return -1;
    }

    // First read temperature file
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

    // Then read humidity file
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
        printf("First entry: {%s: temp=%f, hum=%f}\n", 
               (*dataPoints)[0].timestamp, (*dataPoints)[0].temperature, (*dataPoints)[0].humidity);
        printf("Last entry: {%s: temp=%f, hum=%f}\n", 
               (*dataPoints)[size-1].timestamp, (*dataPoints)[size-1].temperature, (*dataPoints)[size-1].humidity);
    }

    return size;
}

// Binary Interpolation Search implementation
int binaryInterpolationSearch(DataPoint* arr, int size, const char* target_timestamp) {
    int left = 0;
    int right = size - 1;
    
    while (left <= right) {
        int cmp = strcmp(arr[left].timestamp, target_timestamp);
        if (cmp == 0) return left;
        
        cmp = strcmp(arr[right].timestamp, target_timestamp);
        if (cmp == 0) return right;
        
        if (strcmp(target_timestamp, arr[left].timestamp) < 0 ||
            strcmp(target_timestamp, arr[right].timestamp) > 0) {
            return -1;
        }
        
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
    
    int result = binaryInterpolationSearch(dataPoints, size, search_timestamp);
    
    if (result != -1) {
        printf("Found timestamp at index %d:\n", result);
        printf("Temperature: %.1f\n", dataPoints[result].temperature);
        printf("Humidity: %.1f\n", dataPoints[result].humidity);
    } else {
        printf("Timestamp not found\n");
        // Print some entries around where it should be
        for (int i = 0; i < size; i++) {
            if (strcmp(dataPoints[i].timestamp, search_timestamp) > 0) {
                int start = (i - 2 >= 0) ? i - 2 : 0;
                int end = (i + 2 < size) ? i + 2 : size - 1;
                printf("\nNearby entries:\n");
                for (int j = start; j <= end; j++) {
                    printf("%d: {%s: temp=%f, hum=%f}\n", 
                           j, dataPoints[j].timestamp, dataPoints[j].temperature, dataPoints[j].humidity);
                }
                break;
            }
        }
    }

    free(dataPoints);
    return 0;
}