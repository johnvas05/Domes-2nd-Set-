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

typedef struct BSTNode {
    char* date;             // e.g., "2014-02-13"
    double totalTemp;       // sum of all temperatures for the day
    int count;              // number of measurements for the day
    double avgTemp;         // average temperature for the day
    int height; // <-- AVL height
    struct BSTNode* left;
    struct BSTNode* right;
} BSTNode;

BSTNode* root = NULL; // <-- Declare the AVL tree root globally

int readFile(DataPoint** dataPoints) {
    // Open the file directly with the full path
    FILE* file = fopen("tempm.txt", "r");
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

// Utility functions for AVL
int max(int a, int b) { return (a > b) ? a : b; }

int height(BSTNode* n) {
    return n ? n->height : 0;
}

int getBalance(BSTNode* n) {
    return n ? height(n->left) - height(n->right) : 0;
}

BSTNode* rightRotate(BSTNode* y) {
    BSTNode* x = y->left;
    BSTNode* T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;
    return x;
}

BSTNode* leftRotate(BSTNode* x) {
    BSTNode* y = x->right;
    BSTNode* T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;
    return y;
}

// AVL insert
BSTNode* insert(BSTNode* node, const char* date, double temperature) {
    if (!node) {
        BSTNode* n = malloc(sizeof(BSTNode));
        n->date = strdup(date);
        n->totalTemp = temperature;
        n->count = 1;
        n->avgTemp = temperature;
        n->height = 1;
        n->left = n->right = NULL;
        return n;
    }
    int cmp = strcmp(date, node->date);
    if (cmp == 0) {
        node->totalTemp += temperature;
        node->count++;
        node->avgTemp = node->totalTemp / node->count;
        return node;
    } else if (cmp < 0) {
        node->left = insert(node->left, date, temperature);
    } else {
        node->right = insert(node->right, date, temperature);
    }

    node->height = 1 + max(height(node->left), height(node->right));
    int balance = getBalance(node);

    // Left Left
    if (balance > 1 && strcmp(date, node->left->date) < 0)
        return rightRotate(node);

    // Right Right
    if (balance < -1 && strcmp(date, node->right->date) > 0)
        return leftRotate(node);

    // Left Right
    if (balance > 1 && strcmp(date, node->left->date) > 0) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    // Right Left
    if (balance < -1 && strcmp(date, node->right->date) < 0) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

void printBST(BSTNode* root) {
    if (!root) return;
    printBST(root->left);

    // Print header for each record
    printf("----- Record -----\n");
    printf("Date: %s\n", root->date);
    printf("Average Temperature: %.2f°C\n", root->avgTemp);
    printf("Number of measurements: %d\n", root->count);
    printf("------------------\n");

    printBST(root->right);
}

int isBalanced(BSTNode* node) {
    if (!node) return 1;
    int lh = height(node->left);
    int rh = height(node->right);
    int balance = lh - rh;
    if (balance < -1 || balance > 1)
        return 0;
    return isBalanced(node->left) && isBalanced(node->right);
}

// Search for a date in the BST and print the average temperature
void searchByDate(BSTNode* root, const char* date) {
    if (!root) {
        printf("Date %s not found in the records.\n", date);
        return;
    }
    int cmp = strcmp(date, root->date);
    if (cmp == 0) {
        printf("----- Record Found -----\n");
        printf("Date: %s\n", root->date);
        printf("Average Temperature: %.2f°C\n", root->avgTemp);
        printf("Number of measurements: %d\n", root->count);
        printf("------------------------\n");
    } else if (cmp < 0) {
        searchByDate(root->left, date);
    } else {
        searchByDate(root->right, date);
    }
}

// Edit the average temperature for a specific date
void editAvgTemperature(BSTNode* root, const char* date, double newAvg) {
    if (!root) {
        printf("Date %s not found in the records.\n", date);
        return;
    }
    int cmp = strcmp(date, root->date);
    if (cmp == 0) {
        root->avgTemp = newAvg;
        root->totalTemp = newAvg * root->count;
        printf("Average temperature for %s updated to %.2f°C.\n", root->date, root->avgTemp);
    } else if (cmp < 0) {
        editAvgTemperature(root->left, date, newAvg);
    } else {
        editAvgTemperature(root->right, date, newAvg);
    }
}

BSTNode* minValueNode(BSTNode* node) {
    BSTNode* current = node;
    while (current && current->left)
        current = current->left;
    return current;
}

BSTNode* deleteNode(BSTNode* root, const char* date) {
    if (!root) {
        printf("Date %s not found in the records.\n", date);
        return root;
    }
    int cmp = strcmp(date, root->date);
    if (cmp < 0) {
        root->left = deleteNode(root->left, date);
    } else if (cmp > 0) {
        root->right = deleteNode(root->right, date);
    } else {
        // Node with only one child or no child
        if (!root->left || !root->right) {
            BSTNode* temp = root->left ? root->left : root->right;
            free(root->date);
            free(root);
            return temp;
        }
        // Node with two children
        BSTNode* temp = minValueNode(root->right);
        free(root->date);
        root->date = strdup(temp->date);
        root->totalTemp = temp->totalTemp;
        root->count = temp->count;
        root->avgTemp = temp->avgTemp;
        root->right = deleteNode(root->right, temp->date);
    }

    // Update height and balance
    root->height = 1 + max(height(root->left), height(root->right));
    int balance = getBalance(root);

    // Left Left
    if (balance > 1 && getBalance(root->left) >= 0)
        return rightRotate(root);

    // Left Right
    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }

    // Right Right
    if (balance < -1 && getBalance(root->right) <= 0)
        return leftRotate(root);

    // Right Left
    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
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

    // Build the AVL tree from dataPoints
    for (int i = 0; i < dataSize; i++) {
        char* date = extractDate(dataPoints[i].timestamp);
        root = insert(root, date, dataPoints[i].temperature);
        free(date);
    }

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

    int choice;
    char searchDate[11];
    do {
        printf("\nMenu:\n");
        printf("1. Print BST in-order traversal (by date)\n");
        printf("2. Search for average temperature by date\n");
        printf("3. Edit average temperature for a date\n");
        printf("4. Delete a record by date\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // clear invalid input
            continue;
        }
        switch (choice) {
            case 1:
                printf("\nBST In-Order Traversal (by Date):\n");
                printBST(root);
                break;
            case 2:
                printf("Enter a date to search for average temperature (YYYY-MM-DD): ");
                if (scanf("%10s", searchDate) == 1) {
                    searchByDate(root, searchDate);
                }
                break;
            case 3: {
                printf("Enter a date to edit (YYYY-MM-DD): ");
                if (scanf("%10s", searchDate) == 1) {
                    double newAvg;
                    printf("Enter new average temperature: ");
                    if (scanf("%lf", &newAvg) == 1) {
                        editAvgTemperature(root, searchDate, newAvg);
                    } else {
                        printf("Invalid temperature input.\n");
                        while (getchar() != '\n');
                    }
                }
                break;
            }
            case 4:
                printf("Enter a date to delete (YYYY-MM-DD): ");
                if (scanf("%10s", searchDate) == 1) {
                    root = deleteNode(root, searchDate);
                }
                break;
            case 5:
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 5);

    // Free memory
    free(dataPoints);
    freeDailyAverages(dailyAverages, daysCount);

    if (isBalanced(root)) {
        printf("The AVL tree is balanced.\n");
    } else {
        printf("The AVL tree is NOT balanced.\n");
    }

    return 0;
}