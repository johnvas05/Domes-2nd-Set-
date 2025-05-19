#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUCKET_COUNT 11

typedef struct {
    char timestamp[20];
    double temperature;
} DataPoint;

typedef struct BSTNode {
    char* date;
    double totalTemp;
    int count;
    double avgTemp;
    int height;
    struct BSTNode* left;
    struct BSTNode* right;
} BSTNode;

typedef struct HashNode {
    char* date;
    double totalTemp;
    int count;
    double avgTemp;
    struct HashNode* next;
} HashNode;

typedef struct {
    HashNode* buckets[BUCKET_COUNT];
} HashTable;

// --- Utility functions ---
char* extractDate(const char* timestamp) {
    char* date = malloc(11);
    if (!date) return NULL;
    strncpy(date, timestamp, 10);
    date[10] = '\0';
    return date;
}

int hashFunction(const char* date) {
    int sum = 0;
    for (int i = 0; date[i] != '\0'; i++) sum += (int)date[i];
    return sum % BUCKET_COUNT;
}

// --- File reading ---
int readFile(DataPoint** dataPoints) {
    FILE* file = fopen("tempm.txt", "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }
    int size = 0, capacity = 10;
    *dataPoints = malloc(capacity * sizeof(DataPoint));
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
                *dataPoints = realloc(*dataPoints, capacity * sizeof(DataPoint));
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

// --- BST by day ---
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

// Insert by day (date string)
BSTNode* insertByDay(BSTNode* node, const char* date, double temperature) {
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
        node->left = insertByDay(node->left, date, temperature);
    } else {
        node->right = insertByDay(node->right, date, temperature);
    }
    node->height = 1 + max(height(node->left), height(node->right));
    int balance = getBalance(node);
    if (balance > 1 && strcmp(date, node->left->date) < 0)
        return rightRotate(node);
    if (balance < -1 && strcmp(date, node->right->date) > 0)
        return leftRotate(node);
    if (balance > 1 && strcmp(date, node->left->date) > 0) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    if (balance < -1 && strcmp(date, node->right->date) < 0) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
    return node;
}

// Insert by average temperature (use avgTemp as key, break ties with date)
BSTNode* insertByAvg(BSTNode* node, const char* date, double avgTemp, double totalTemp, int count) {
    if (!node) {
        BSTNode* n = malloc(sizeof(BSTNode));
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
        node->left = insertByAvg(node->left, date, avgTemp, totalTemp, count);
    } else if (avgTemp > node->avgTemp ||
               (avgTemp == node->avgTemp && strcmp(date, node->date) > 0)) {
        node->right = insertByAvg(node->right, date, avgTemp, totalTemp, count);
    } else {
        // Duplicate (same avgTemp and date), do nothing
        return node;
    }
    node->height = 1 + max(height(node->left), height(node->right));
    int balance = getBalance(node);
    if (balance > 1 && (avgTemp < node->left->avgTemp ||
        (avgTemp == node->left->avgTemp && strcmp(date, node->left->date) < 0)))
        return rightRotate(node);
    if (balance < -1 && (avgTemp > node->right->avgTemp ||
        (avgTemp == node->right->avgTemp && strcmp(date, node->right->date) > 0)))
        return leftRotate(node);
    if (balance > 1 && (avgTemp > node->left->avgTemp ||
        (avgTemp == node->left->avgTemp && strcmp(date, node->left->date) > 0))) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
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
    printf("Date: %s, Average Temperature: %.2f°C, Measurements: %d\n",
           root->date, root->avgTemp, root->count);
    printBST(root->right);
}

void freeBST(BSTNode* root) {
    if (!root) return;
    freeBST(root->left);
    freeBST(root->right);
    free(root->date);
    free(root);
}

// --- Calculate daily averages for BST by avg ---
typedef struct {
    char* date;
    double totalTemp;
    int count;
    double avgTemp;
} DailyAverage;

void freeDailyAverages(DailyAverage* dailyAvgs, int size) {
    if (!dailyAvgs) return;
    for (int i = 0; i < size; i++) free(dailyAvgs[i].date);
    free(dailyAvgs);
}

int calculateDailyAverages(DataPoint* dataPoints, int size, DailyAverage** dailyAvgs) {
    if (size <= 0) return 0;
    int dailyCount = 0, dailyCapacity = 10;
    *dailyAvgs = malloc(dailyCapacity * sizeof(DailyAverage));
    if (!*dailyAvgs) return -1;
    for (int i = 0; i < size; i++) {
        char* currentDate = extractDate(dataPoints[i].timestamp);
        if (!currentDate) {
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
                *dailyAvgs = realloc(*dailyAvgs, dailyCapacity * sizeof(DailyAverage));
                if (!*dailyAvgs) {
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
    for (int i = 0; i < dailyCount; i++)
        (*dailyAvgs)[i].avgTemp = (*dailyAvgs)[i].totalTemp / (*dailyAvgs)[i].count;
    return dailyCount;
}

// --- Chain Hashing ---
HashNode* createHashNode(const char* date, double temperature) {
    HashNode* newNode = malloc(sizeof(HashNode));
    if (!newNode) return NULL;
    newNode->date = strdup(date);
    newNode->totalTemp = temperature;
    newNode->count = 1;
    newNode->avgTemp = temperature;
    newNode->next = NULL;
    return newNode;
}
void insertHashTable(HashTable* table, const char* date, double temperature) {
    int index = hashFunction(date);
    HashNode* current = table->buckets[index];
    while (current) {
        if (strcmp(current->date, date) == 0) {
            current->totalTemp += temperature;
            current->count++;
            current->avgTemp = current->totalTemp / current->count;
            return;
        }
        current = current->next;
    }
    HashNode* newNode = createHashNode(date, temperature);
    if (!newNode) return;
    newNode->next = table->buckets[index];
    table->buckets[index] = newNode;
}
void printHashTable(HashTable* table) {
    for (int i = 0; i < BUCKET_COUNT; i++) {
        HashNode* current = table->buckets[i];
        while (current) {
            printf("Date: %s, Average Temperature: %.2f°C, Measurements: %d\n",
                   current->date, current->avgTemp, current->count);
            current = current->next;
        }
    }
}
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

// --- Search by date ---
void searchByDate(BSTNode* root, const char* date) {
    if (!root) {
        printf("No records found for the given date.\n");
        return;
    }
    int cmp = strcmp(date, root->date);
    if (cmp == 0) {
        printf("Date: %s, Average Temperature: %.2f°C, Measurements: %d\n",
               root->date, root->avgTemp, root->count);
    } else if (cmp < 0) {
        searchByDate(root->left, date);
    } else {
        searchByDate(root->right, date);
    }
}

// --- Search by date for HashTable ---
void searchByDateHash(HashTable* table, const char* date) {
    int index = hashFunction(date);
    HashNode* current = table->buckets[index];
    while (current) {
        if (strcmp(current->date, date) == 0) {
            printf("Date: %s, Average Temperature: %.2f°C, Measurements: %d\n",
                   current->date, current->avgTemp, current->count);
            return;
        }
        current = current->next;
    }
    printf("No records found for the given date.\n");
}

// --- Edit average temperature ---
void editAvgTemperature(BSTNode* root, const char* date, double newAvg) {
    if (!root) return;
    int cmp = strcmp(date, root->date);
    if (cmp == 0) {
        double diff = newAvg - root->avgTemp;
        root->totalTemp += diff * root->count;
        root->avgTemp = newAvg;
    } else if (cmp < 0) {
        editAvgTemperature(root->left, date, newAvg);
    } else {
        editAvgTemperature(root->right, date, newAvg);
    }
}

// --- Delete a record by date ---
BSTNode* deleteNode(BSTNode* root, const char* date) {
    if (!root) return NULL;
    int cmp = strcmp(date, root->date);
    if (cmp == 0) {
        // Node to be deleted found
        if (!root->left && !root->right) {
            // Case 1: No children (leaf node)
            free(root->date);
            free(root);
            return NULL;
        } else if (!root->left) {
            // Case 2: One child (right)
            BSTNode* temp = root->right;
            free(root->date);
            free(root);
            return temp;
        } else if (!root->right) {
            // Case 2: One child (left)
            BSTNode* temp = root->left;
            free(root->date);
            free(root);
            return temp;
        } else {
            // Case 3: Two children
            // Find the minimum value node in the right subtree (in-order successor)
            BSTNode* minNode = root->right;
            while (minNode->left) minNode = minNode->left;
            // Replace root's data with the in-order successor's data
            free(root->date);
            root->date = strdup(minNode->date);
            root->totalTemp = minNode->totalTemp;
            root->count = minNode->count;
            root->avgTemp = minNode->avgTemp;
            // Delete the in-order successor
            root->right = deleteNode(root->right, minNode->date);
        }
    } else if (cmp < 0) {
        root->left = deleteNode(root->left, date);
    } else {
        root->right = deleteNode(root->right, date);
    }
    root->height = 1 + max(height(root->left), height(root->right));
    int balance = getBalance(root);
    if (balance > 1 && strcmp(date, root->left->date) < 0)
        return rightRotate(root);
    if (balance < -1 && strcmp(date, root->right->date) > 0)
        return leftRotate(root);
    if (balance > 1 && strcmp(date, root->left->date) > 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }
    if (balance < -1 && strcmp(date, root->right->date) < 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }
    return root;
}

// --- Find minimum average temperature ---
double findMinAvg(BSTNode* root) {
    if (!root) return -1;
    while (root->left) root = root->left;
    return root->avgTemp;
}

// --- Find maximum average temperature ---
double findMaxAvg(BSTNode* root) {
    if (!root) return -1;
    while (root->right) root = root->right;
    return root->avgTemp;
}

// --- Find day(s) by average temperature ---
int findDaysByAvg(BSTNode* root, double targetAvg) {
    if (!root) return 0;
    int found = 0;
    if (root->avgTemp == targetAvg) {
        printf("Date: %s, Average Temperature: %.2f°C, Measurements: %d\n",
               root->date, root->avgTemp, root->count);
        found = 1;
    }
    found += findDaysByAvg(root->left, targetAvg);
    found += findDaysByAvg(root->right, targetAvg);
    return found;
}

// --- BST Menu (by day) ---
void bstMenuByDay(BSTNode* root) {
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
            while (getchar() != '\n');
            break;
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
}

// --- BST Menu (by average temperature) ---
void bstMenuByAvg(BSTNode* root) {
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
}

// --- Chain Hashing Menu ---
void chainHashingMenu(HashTable* table) {
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
            while (getchar() != '\n');
            continue;
        }
        switch (choice) {
            case 1:
                printf("Enter date (YYYY-MM-DD): ");
                if (scanf("%10s", searchDate) == 1) {
                    searchByDateHash(table, searchDate);
                }
                break;
            case 2: {
                printf("Enter date (YYYY-MM-DD): ");
                if (scanf("%10s", searchDate) == 1) {
                    double newAvg;
                    printf("Enter new average temperature: ");
                    if (scanf("%lf", &newAvg) == 1) {
                        // You need to implement editAvgTemperature for HashTable if not already present
                        // For now, print a message
                        printf("Edit average temperature not implemented for HashTable.\n");
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
                    // You need to implement deleteFromHashTable if not already present
                    // For now, print a message
                    printf("Delete record not implemented for HashTable.\n");
                }
                break;
            case 4:
                printf("Exiting application.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 4);
}

// --- Main ---
int main() {
    DataPoint* dataPoints = NULL;
    int dataSize = readFile(&dataPoints);
    if (dataSize == -1) return 1;

    printf("Choose data structure to load data:\n");
    printf("1. BST\n");
    printf("2. Chain Hashing\n");
    int structureChoice;
    do {
        printf("Enter choice (1 or 2): ");
        if (scanf("%d", &structureChoice) != 1) {
            while (getchar() != '\n');
            continue;
        }
    } while (structureChoice != 1 && structureChoice != 2);

    if (structureChoice == 1) {
        printf("Choose BST loading method:\n");
        printf("1. By day\n");
        printf("2. By average temperature of the day\n");
        int bstChoice;
        do {
            printf("Enter choice (1 or 2): ");
            if (scanf("%d", &bstChoice) != 1) {
                while (getchar() != '\n');
                continue;
            }
        } while (bstChoice != 1 && bstChoice != 2);

        BSTNode* root = NULL;
        if (bstChoice == 1) {
            // BST by day
            for (int i = 0; i < dataSize; i++) {
                char* date = extractDate(dataPoints[i].timestamp);
                root = insertByDay(root, date, dataPoints[i].temperature);
                free(date);
            }
            bstMenuByDay(root);
            freeBST(root);
        } else {
            // BST by average temperature
            DailyAverage* dailyAvgs = NULL;
            int daysCount = calculateDailyAverages(dataPoints, dataSize, &dailyAvgs);
            if (daysCount == -1) {
                free(dataPoints);
                return 1;
            }
            for (int i = 0; i < daysCount; i++) {
                root = insertByAvg(root, dailyAvgs[i].date, dailyAvgs[i].avgTemp,
                                   dailyAvgs[i].totalTemp, dailyAvgs[i].count);
            }
            bstMenuByAvg(root);
            freeBST(root);
            freeDailyAverages(dailyAvgs, daysCount);
        }
    } else {
        // Chain Hashing
        HashTable table = {0};
        for (int i = 0; i < dataSize; i++) {
            char* date = extractDate(dataPoints[i].timestamp);
            insertHashTable(&table, date, dataPoints[i].temperature);
            free(date);
        }
        chainHashingMenu(&table);
        freeHashTable(&table);
    }

    free(dataPoints);
    return 0;
}