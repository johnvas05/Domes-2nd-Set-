#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 10000
#define MAX_DAY  20

typedef struct DayTemp {
    char *day; // YYYY-MM-DD format
    double sum; // sum of temperatures
    int count;
    struct DayTemp *left, *right;
} DayTemp;

DayTemp* create_node(const char* day, double temp) {
    DayTemp* node = (DayTemp*)malloc(sizeof(DayTemp));
    node->day = (char*)malloc(strlen(day) + 1);
    strcpy(node->day, day);
    node->sum = temp;
    node->count = 1;
    node->left = node->right = NULL;
    return node;
}

// Insert or update node in BST
DayTemp* insert(DayTemp* root, const char* day, double temp) {
    if (!root) return create_node(day, temp);
    int cmp = strcmp(day, root->day);
    if (cmp == 0) {
        root->sum += temp;
        root->count += 1;
    } else if (cmp < 0) {
        root->left = insert(root->left, day, temp);
    } else {
        root->right = insert(root->right, day, temp);
    }
    return root;
}

// Print BST in-order
void print_bst(DayTemp* root) {
    if (!root) return;
    print_bst(root->left);
    printf("%s: %.2f\n", root->day, root->sum / root->count);
    print_bst(root->right);
}

// Free BST
void free_bst(DayTemp* root) {
    if (!root) return;
    free_bst(root->left);
    free_bst(root->right);
    free(root->day); // free the day string
    free(root);
}

// Extract date (YYYY-MM-DD) from timestamp
void extract_day(const char* timestamp, char* day_out) {
    strncpy(day_out, timestamp, 10);
    day_out[10] = '\0';
}

// Parse the JSON line and update BST
void parse_line(char* line, DayTemp** root) {
    char *p = line;
    while ((p = strchr(p, '\"'))) {
        char timestamp[25], value[20], day[MAX_DAY];
        p++; // skip opening quote
        char *q = strchr(p, '\"');
        if (!q) break;
        int len = q - p;
        strncpy(timestamp, p, len);
        timestamp[len] = '\0';
        p = strchr(q + 1, ':');
        if (!p) break;
        p++; // skip colon
        while (*p && (isspace(*p) || *p == '\"')) p++;
        char *vstart = p;
        char *vend = vstart;
        while (*vend && *vend != '\"' && *vend != ',' && *vend != '}') vend++;
        int vlen = vend - vstart;
        strncpy(value, vstart, vlen);
        value[vlen] = '\0';
        p = vend;
        if (strlen(value) == 0) continue; // skip empty values
        extract_day(timestamp, day);
        double temp = atof(value);
        *root = insert(*root, day, temp);
    }
}

int main() {
    FILE *fp = fopen("tempm.txt", "r");
    if (!fp) {
        printf("Could not open tempm.txt\n");
        return 1;
    }
    char line[MAX_LINE];
    DayTemp* root = NULL;
    while (fgets(line, sizeof(line), fp)) {
        parse_line(line, &root);
    }
    fclose(fp);

    printf("Average temperature per day:\n");
    print_bst(root);
    free_bst(root);
    return 0;
}