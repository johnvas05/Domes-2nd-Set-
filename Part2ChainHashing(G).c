#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define the DataPoint structure
typedef struct {
    char timestamp[20]; // e.g., "2014-02-13T12:00:00"
    double temperature; // Temperature value
} DataPoint;

#define BUCKET_COUNT 11 // Number of buckets (odd number)

typedef struct HashNode {
    char* date;             // e.g., "2014-02-13"
    double totalTemp;       // Sum of all temperatures for the day
    int count;              // Number of measurements for the day
    double avgTemp;         // Average temperature for the day
    struct HashNode* next;  // Pointer to the next node in the chain
} HashNode;

typedef struct {
    HashNode* buckets[BUCKET_COUNT]; // Array of bucket pointers
} HashTable;

// Hash function to calculate the bucket index
int hashFunction(const char* date) {
    int sum = 0;
    for (int i = 0; date[i] != '\0'; i++) {
        sum += (int)date[i];
    }
    return sum % BUCKET_COUNT;
}

// Create a new hash node
HashNode* createHashNode(const char* date, double temperature) {
    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
    if (!newNode) {
        perror("Failed to allocate memory for hash node");
        return NULL;
    }
    newNode->date = strdup(date);
    newNode->totalTemp = temperature;
    newNode->count = 1;
    newNode->avgTemp = temperature;
    newNode->next = NULL;
    return newNode;
}

// Insert or update a date in the hash table
void insertHashTable(HashTable* table, const char* date, double temperature) {
    int index = hashFunction(date);
    HashNode* current = table->buckets[index];

    // Search for the date in the chain
    while (current) {
        if (strcmp(current->date, date) == 0) {
            // Update the existing node
            current->totalTemp += temperature;
            current->count++;
            current->avgTemp = current->totalTemp / current->count;
            return;
        }
        current = current->next;
    }

    // If not found, create a new node and insert it at the head of the chain
    HashNode* newNode = createHashNode(date, temperature);
    if (!newNode) return;
    newNode->next = table->buckets[index];
    table->buckets[index] = newNode;
}

// Search for a date in the hash table
HashNode* searchHashTable(HashTable* table, const char* date) {
    int index = hashFunction(date);
    HashNode* current = table->buckets[index];

    while (current) {
        if (strcmp(current->date, date) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Delete a date from the hash table
void deleteFromHashTable(HashTable* table, const char* date) {
    int index = hashFunction(date);
    HashNode* current = table->buckets[index];
    HashNode* prev = NULL;

    while (current) {
        if (strcmp(current->date, date) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                table->buckets[index] = current->next;
            }
            free(current->date);
            free(current);
            printf("Date %s deleted successfully.\n", date);
            return;
        }
        prev = current;
        current = current->next;
    }
    printf("Date %s not found in the records.\n", date);
}

// Print the hash table
void printHashTable(HashTable* table) {
    for (int i = 0; i < BUCKET_COUNT; i++) {
        printf("Bucket %d:\n", i);
        HashNode* current = table->buckets[i];
        while (current) {
            printf("  Date: %s, Average Temperature: %.2f°C, Measurements: %d\n",
                   current->date, current->avgTemp, current->count);
            current = current->next;
        }
    }
}

// Free the hash table
void freeHashTable(HashTable* table) {
    for (int i = 0; i < BUCKET_COUNT; i++) {
        HashNode* current = table->buckets[i];
        while (current) {
            HashNode* temp = current;
            current = current->next;
            free(temp->date);
            free(temp);
        }
    }
}

// Extract the date from a timestamp
char* extractDate(const char* timestamp) {
    char* date = (char*)malloc(11 * sizeof(char)); // 10 for "YYYY-MM-DD" + 1 for '\0'
    if (!date) {
        perror("Failed to allocate memory for date");
        return NULL;
    }
    strncpy(date, timestamp, 10);
    date[10] = '\0';
    return date;
}

// Read data from the file
int readFile(DataPoint** dataPoints) {
    FILE* file = fopen("tempm.txt", "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    int size = 0, capacity = 10;
    *dataPoints = (DataPoint*)malloc(capacity * sizeof(DataPoint));
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
            if (size >= capacity) {
                capacity *= 2;
                *dataPoints = (DataPoint*)realloc(*dataPoints, capacity * sizeof(DataPoint));
                if (!*dataPoints) {
                    perror("Memory reallocation failed");
                    fclose(file);
                    return -1;
                }
            }
            strcpy((*dataPoints)[size].timestamp, timestamp);
            (*dataPoints)[size].temperature = temperature;
            size++;
        }
    }

    fclose(file);
    return size;
}

// Search for average temperature by date
void searchByDate(HashTable* table, const char* date) {
    HashNode* node = searchHashTable(table, date);
    if (node) {
        printf("Date: %s\n", node->date);
        printf("Average Temperature: %.2f°C\n", node->avgTemp);
        printf("Number of measurements: %d\n", node->count);
    } else {
        printf("Date %s not found in the records.\n", date);
    }
}

// Edit average temperature by date
void editAvgTemperature(HashTable* table, const char* date, double newAvg) {
    HashNode* node = searchHashTable(table, date);
    if (node) {
        node->avgTemp = newAvg;
        node->totalTemp = newAvg * node->count;
        printf("Average temperature for %s updated to %.2f°C.\n", node->date, node->avgTemp);
    } else {
        printf("Date %s not found in the records.\n", date);
    }
}

// Function to write hash table contents to a file
void writeHashTableToFile(HashTable* table, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Error opening output file");
        return;
    }

    fprintf(file, "Temperature Records Export\n");
    fprintf(file, "========================\n\n");

    for (int i = 0; i < BUCKET_COUNT; i++) {
        HashNode* current = table->buckets[i];
        while (current) {
            fprintf(file, "Date: %s\n", current->date);
            fprintf(file, "Average Temperature: %.2f°C\n", current->avgTemp);
            fprintf(file, "Number of measurements: %d\n", current->count);
            fprintf(file, "------------------------\n");
            current = current->next;
        }
    }

    fclose(file);
    printf("Results have been exported to %s\n", filename);
}

int main() {
    DataPoint* dataPoints = NULL;
    HashTable table = {0}; // Initialize the hash table with NULL buckets

    // Read data from the file
    int dataSize = readFile(&dataPoints);
    if (dataSize == -1) {
        return 1;
    }

    // Insert data into the hash table
    for (int i = 0; i < dataSize; i++) {
        char* date = extractDate(dataPoints[i].timestamp);
        if (date) {
            insertHashTable(&table, date, dataPoints[i].temperature);
            free(date);
        }
    }

    int choice;
    char searchDate[11];
    do {
        printf("\nMenu:\n");
        printf("1. Search for AVERAGE TEMPERATURE by DATE\n");
        printf("2. Edit the average temperature by DATE\n");
        printf("3. Delete a record by DATE\n");
        printf("4. Exit\n");
        printf("Choice: ");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // clear invalid input
            continue;
        }
        switch (choice) {
            case 1:
                printf("Enter date (YYYY-MM-DD): ");
                if (scanf("%10s", searchDate) == 1) {
                    searchByDate(&table, searchDate);
                }
                break;
            case 2: {
                printf("Enter date (YYYY-MM-DD): ");
                if (scanf("%10s", searchDate) == 1) {
                    double newAvg;
                    printf("Enter new average temperature: ");
                    if (scanf("%lf", &newAvg) == 1) {
                        editAvgTemperature(&table, searchDate, newAvg);
                    } else {
                        printf("Invalid temperature value.\n");
                        while (getchar() != '\n');
                    }
                }
                break;
            }
            case 3:
                printf("Enter date (YYYY-MM-DD): ");
                if (scanf("%10s", searchDate) == 1) {
                    deleteFromHashTable(&table, searchDate);
                }
                break;
            case 4:
                printf("Exporting results before exit...\n");
                writeHashTableToFile(&table, "HashTableResults.txt");
                printf("Exiting application.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 4);

    // Free memory
    free(dataPoints);
    freeHashTable(&table);

    return 0;
}