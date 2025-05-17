#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char timestamp[20];
    double temperature;
} DataPoint;

typedef struct {
    char* date;
    double totalTemp;
    int count;
    double avgTemp;
} DailyAverage;

typedef struct BSTNode {
    char* date;
    double totalTemp;
    int count;
    double avgTemp;
    int height;
    struct BSTNode* left;
    struct BSTNode* right;
} BSTNode;

BSTNode* root = NULL;

int max(int a, int b) { return (a > b) ? a : b; }

int height(BSTNode* n) { return n ? n->height : 0; }

int getBalance(BSTNode* n) { return n ? height(n->left) - height(n->right) : 0; }

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

// --- Read file and calculate daily averages ---
int readFile(DataPoint** dataPoints) {
    FILE* file = fopen("tempm.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

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

char* extractDate(const char* timestamp) {
    char* date = (char*)malloc(11 * sizeof(char));
    if (date == NULL) {
        perror("Failed to allocate memory for date");
        return NULL;
    }
    strncpy(date, timestamp, 10);
    date[10] = '\0';
    return date;
}

void freeDailyAverages(DailyAverage* dailyAvgs, int size) {
    if (dailyAvgs == NULL) return;
    for (int i = 0; i < size; i++) {
        free(dailyAvgs[i].date);
    }
    free(dailyAvgs);
}

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
        int found = 0;
        for (int j = 0; j < dailyCount; j++) {
            if (strcmp((*dailyAvgs)[j].date, currentDate) == 0) {
                (*dailyAvgs)[j].totalTemp += dataPoints[i].temperature;
                (*dailyAvgs)[j].count++;
                found = 1;
                free(currentDate);
                break;
            }
        }
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
    for (int i = 0; i < dailyCount; i++) {
        (*dailyAvgs)[i].avgTemp = (*dailyAvgs)[i].totalTemp / (*dailyAvgs)[i].count;
    }
    return dailyCount;
}

// --- BST by average temperature ---
BSTNode* insert(BSTNode* node, const char* date, double avgTemp, double totalTemp, int count) {
    if (!node) {
        BSTNode* n = (BSTNode*)malloc(sizeof(BSTNode));
        n->date = strdup(date);
        n->avgTemp = avgTemp;
        n->totalTemp = totalTemp;
        n->count = count;
        n->height = 1;
        n->left = n->right = NULL;
        return n;
    }
    if (avgTemp < node->avgTemp ||
        (avgTemp == node->avgTemp && strcmp(date, node->date) < 0)) {
        node->left = insert(node->left, date, avgTemp, totalTemp, count);
    } else if (avgTemp > node->avgTemp ||
               (avgTemp == node->avgTemp && strcmp(date, node->date) > 0)) {
        node->right = insert(node->right, date, avgTemp, totalTemp, count);
    } else {
        // Duplicate (same avgTemp and date), do nothing
        return node;
    }

    node->height = 1 + max(height(node->left), height(node->right));
    int balance = getBalance(node);

    // Left Left
    if (balance > 1 && (avgTemp < node->left->avgTemp ||
        (avgTemp == node->left->avgTemp && strcmp(date, node->left->date) < 0)))
        return rightRotate(node);

    // Right Right
    if (balance < -1 && (avgTemp > node->right->avgTemp ||
        (avgTemp == node->right->avgTemp && strcmp(date, node->right->date) > 0)))
        return leftRotate(node);

    // Left Right
    if (balance > 1 && (avgTemp > node->left->avgTemp ||
        (avgTemp == node->left->avgTemp && strcmp(date, node->left->date) > 0))) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    // Right Left
    if (balance < -1 && (avgTemp < node->right->avgTemp ||
        (avgTemp == node->right->avgTemp && strcmp(date, node->right->date) < 0))) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

void printBST(BSTNode* root) {
    if (!root) return;
    printBST(root->left);
    printf("----- Record -----\n");
    printf("Date: %s\n", root->date);
    printf("Average Temperature: %.2f°C\n", root->avgTemp);
    printf("Number of measurements: %d\n", root->count);
    printf("------------------\n");
    printBST(root->right);
}

void printAllDays(BSTNode* root) {
    if (!root) return;
    printAllDays(root->left);
    printf("Date: %s, Average Temperature: %.2f°C, Measurements: %d\n",
           root->date, root->avgTemp, root->count);
    printAllDays(root->right);
}

void searchByAvg(BSTNode* root, double avgTemp) {
    if (!root) {
        printf("No record found with average temperature %.2f°C.\n", avgTemp);
        return;
    }
    if (avgTemp < root->avgTemp)
        searchByAvg(root->left, avgTemp);
    else if (avgTemp > root->avgTemp)
        searchByAvg(root->right, avgTemp);
    else {
        printf("----- Record Found -----\n");
        printf("Date: %s\n", root->date);
        printf("Average Temperature: %.2f°C\n", root->avgTemp);
        printf("Number of measurements: %d\n", root->count);
        printf("------------------------\n");
    }
}

void editAvgTemperature(BSTNode* root, double avgTemp, double newAvg) {
    if (!root) {
        printf("No record found with average temperature %.2f°C.\n", avgTemp);
        return;
    }
    if (avgTemp < root->avgTemp)
        editAvgTemperature(root->left, avgTemp, newAvg);
    else if (avgTemp > root->avgTemp)
        editAvgTemperature(root->right, avgTemp, newAvg);
    else {
        root->avgTemp = newAvg;
        root->totalTemp = newAvg * root->count;
        printf("Average temperature for %s updated to %.2f°C.\n", root->date, root->avgTemp);
    }
}

BSTNode* minValueNode(BSTNode* node) {
    BSTNode* current = node;
    while (current && current->left)
        current = current->left;
    return current;
}

BSTNode* deleteNode(BSTNode* root, double avgTemp, const char* date) {
    if (!root) {
        printf("No record found with average temperature %.2f°C and date %s.\n", avgTemp, date);
        return root;
    }
    if (avgTemp < root->avgTemp ||
        (avgTemp == root->avgTemp && strcmp(date, root->date) < 0)) {
        root->left = deleteNode(root->left, avgTemp, date);
    } else if (avgTemp > root->avgTemp ||
               (avgTemp == root->avgTemp && strcmp(date, root->date) > 0)) {
        root->right = deleteNode(root->right, avgTemp, date);
    } else {
        // Node found
        if (!root->left || !root->right) {
            BSTNode* temp = root->left ? root->left : root->right;
            free(root->date);
            free(root);
            return temp;
        }
        BSTNode* temp = minValueNode(root->right);
        free(root->date);
        root->date = strdup(temp->date);
        root->avgTemp = temp->avgTemp;
        root->totalTemp = temp->totalTemp;
        root->count = temp->count;
        root->right = deleteNode(root->right, temp->avgTemp, temp->date);
    }

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

void freeBST(BSTNode* root) {
    if (!root) return;
    freeBST(root->left);
    freeBST(root->right);
    free(root->date);
    free(root);
}

int findDaysByAvg(BSTNode* root, double avgTemp) {
    if (!root) return 0;
    int count = 0;
    count += findDaysByAvg(root->left, avgTemp);
    if (root->avgTemp == avgTemp) {
        printf("----- Record -----\n");
        printf("Date: %s\n", root->date);
        printf("Average Temperature: %.2f°C\n", root->avgTemp);
        printf("Number of measurements: %d\n", root->count);
        printf("------------------\n");
        count++;
    }
    count += findDaysByAvg(root->right, avgTemp);
    return count;
}

double findMinAvg(BSTNode* root) {
    if (!root) return -1;
    BSTNode* current = root;
    while (current->left) current = current->left;
    return current->avgTemp;
}

double findMaxAvg(BSTNode* root) {
    if (!root) return -1;
    BSTNode* current = root;
    while (current->right) current = current->right;
    return current->avgTemp;
}

int main() {
    DataPoint* dataPoints = NULL;
    DailyAverage* dailyAverages = NULL;

    int dataSize = readFile(&dataPoints);
    if (dataSize == -1) return 1;

    int daysCount = calculateDailyAverages(dataPoints, dataSize, &dailyAverages);
    if (daysCount == -1) {
        free(dataPoints);
        return 1;
    }

    // Build BST by average temperature
    for (int i = 0; i < daysCount; i++) {
        root = insert(root, dailyAverages[i].date, dailyAverages[i].avgTemp,
                      dailyAverages[i].totalTemp, dailyAverages[i].count);
    }

    // Print all days and their average temperatures
    printf("\nAll days and their average temperatures (in-order):\n");
    printAllDays(root);

    int choice;
    do {
        printf("\nMenu:\n");
        printf("1. Find day(s) by minimum average temperature\n");
        printf("2. Find day(s) by maximum average temperature\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        switch (choice) {
            case 1: {
                double minAvg = findMinAvg(root);
                if (minAvg == -1) {
                    printf("No records found.\n");
                    break;
                }
                printf("Minimum average temperature is %.2f°C.\n", minAvg);
                printf("Day(s) with minimum average temperature:\n");
                int found = findDaysByAvg(root, minAvg);
                if (found == 0) {
                    printf("No days found with that average temperature.\n");
                }
                break;
            }
            case 2: {
                double maxAvg = findMaxAvg(root);
                if (maxAvg == -1) {
                    printf("No records found.\n");
                    break;
                }
                printf("Maximum average temperature is %.2f°C.\n", maxAvg);
                printf("Day(s) with maximum average temperature:\n");
                int found = findDaysByAvg(root, maxAvg);
                if (found == 0) {
                    printf("No days found with that average temperature.\n");
                }
                break;
            }
            case 0:
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);

    free(dataPoints);
    freeDailyAverages(dailyAverages, daysCount);
    freeBST(root);
    return 0;
}