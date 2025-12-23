#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORDER 3
#define MAX_KEYS ORDER
#define MIN_KEYS ((ORDER + 1) / 2)

typedef struct BPlusNode {
    int keys[MAX_KEYS + 1];
    int numKeys;
    int isLeaf;
    struct BPlusNode* children[MAX_KEYS + 2];
    struct BPlusNode* next;
    struct BPlusNode* parent;
} BPlusNode;

typedef struct BPlusTree {
    BPlusNode* root;
} BPlusTree;

// Function prototypes
BPlusTree* createTree();
BPlusNode* createNode(int isLeaf);
void insert(BPlusTree* tree, int key);
BPlusNode* insertInternal(BPlusNode* node, int key, BPlusNode* child);
BPlusNode* findLeaf(BPlusNode* node, int key);
int search(BPlusTree* tree, int key);
void saveTree(BPlusTree* tree, const char* filename);
void saveNodeToFile(FILE* file, BPlusNode* node, int depth);
void printAllValues(FILE* file, BPlusTree* tree);
BPlusTree* loadTree(const char* filename);
void freeTree(BPlusNode* node);
void closeTree(BPlusTree* tree);
char* urlDecode(char* str);
void parsePostData(char* data, char* action, int* value);
void generateHTMLTree(BPlusTree* tree, const char* filename);
void printNodeHTML(FILE* file, BPlusNode* node, int level);
void printAllValuesHTML(FILE* file, BPlusTree* tree);

BPlusTree* createTree() {
    BPlusTree* tree = (BPlusTree*)malloc(sizeof(BPlusTree));
    if (!tree) return NULL;
    tree->root = createNode(1);
    return tree;
}

BPlusNode* createNode(int isLeaf) {
    BPlusNode* node = (BPlusNode*)malloc(sizeof(BPlusNode));
    if (!node) return NULL;

    node->numKeys = 0;
    node->isLeaf = isLeaf;
    node->next = NULL;
    node->parent = NULL;

    for (int i = 0; i < MAX_KEYS + 2; i++) {
        node->children[i] = NULL;
    }

    return node;
}

BPlusNode* findLeaf(BPlusNode* node, int key) {
    if (node->isLeaf) {
        return node;
    }

    int i = 0;
    while (i < node->numKeys && key >= node->keys[i]) {
        i++;
    }

    return findLeaf(node->children[i], key);
}

void insert(BPlusTree* tree, int key) {
    if (!tree || !tree->root) return;

    BPlusNode* leaf = findLeaf(tree->root, key);

    for (int i = 0; i < leaf->numKeys; i++) {
        if (leaf->keys[i] == key) {
            return;
        }
    }

    int i = leaf->numKeys - 1;
    while (i >= 0 && leaf->keys[i] > key) {
        leaf->keys[i + 1] = leaf->keys[i];
        i--;
    }
    leaf->keys[i + 1] = key;
    leaf->numKeys++;

    if (leaf->numKeys > MAX_KEYS) {
        BPlusNode* newLeaf = createNode(1);
        int mid = (MAX_KEYS + 1) / 2;

        int j = 0;
        for (int i = mid; i < leaf->numKeys; i++) {
            newLeaf->keys[j++] = leaf->keys[i];
        }
        newLeaf->numKeys = j;
        leaf->numKeys = mid;

        newLeaf->next = leaf->next;
        leaf->next = newLeaf;

        int promoteKey = newLeaf->keys[0];

        if (leaf->parent == NULL) {
            BPlusNode* newRoot = createNode(0);
            newRoot->keys[0] = promoteKey;
            newRoot->children[0] = leaf;
            newRoot->children[1] = newLeaf;
            newRoot->numKeys = 1;

            leaf->parent = newRoot;
            newLeaf->parent = newRoot;
            tree->root = newRoot;
        }
        else {
            newLeaf->parent = leaf->parent;
            insertInternal(leaf->parent, promoteKey, newLeaf);
        }
    }
}

BPlusNode* insertInternal(BPlusNode* node, int key, BPlusNode* child) {
    int i = node->numKeys - 1;
    while (i >= 0 && node->keys[i] > key) {
        node->keys[i + 1] = node->keys[i];
        node->children[i + 2] = node->children[i + 1];
        i--;
    }
    node->keys[i + 1] = key;
    node->children[i + 2] = child;
    node->numKeys++;

    if (node->numKeys > MAX_KEYS) {
        BPlusNode* newInternal = createNode(0);
        int mid = (MAX_KEYS + 1) / 2;

        int promoteKey = node->keys[mid];

        int j = 0;
        for (int i = mid + 1; i < node->numKeys; i++) {
            newInternal->keys[j] = node->keys[i];
            newInternal->children[j] = node->children[i];
            if (node->children[i]) {
                node->children[i]->parent = newInternal;
            }
            j++;
        }
        newInternal->children[j] = node->children[node->numKeys];
        if (node->children[node->numKeys]) {
            node->children[node->numKeys]->parent = newInternal;
        }

        newInternal->numKeys = j;
        node->numKeys = mid;

        if (node->parent == NULL) {
            BPlusNode* newRoot = createNode(0);
            newRoot->keys[0] = promoteKey;
            newRoot->children[0] = node;
            newRoot->children[1] = newInternal;
            newRoot->numKeys = 1;

            node->parent = newRoot;
            newInternal->parent = newRoot;

            return newRoot;
        }
        else {
            newInternal->parent = node->parent;
            return insertInternal(node->parent, promoteKey, newInternal);
        }
    }

    return node;
}

int search(BPlusTree* tree, int key) {
    if (!tree || !tree->root) return 0;

    BPlusNode* leaf = findLeaf(tree->root, key);

    for (int i = 0; i < leaf->numKeys; i++) {
        if (leaf->keys[i] == key) {
            return 1;
        }
    }

    return 0;
}

void saveTree(BPlusTree* tree, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) return;

    fprintf(file, "========================================\n");
    fprintf(file, "B+ TREE STRUCTURE (Order %d)\n", ORDER);
    fprintf(file, "Max %d keys per node, Min %d keys\n", MAX_KEYS, MIN_KEYS - 1);
    fprintf(file, "========================================\n\n");

    if (tree && tree->root) {
        saveNodeToFile(file, tree->root, 0);
    }
    else {
        fprintf(file, "Tree is empty.\n");
    }

    fprintf(file, "\n========================================\n");
    fprintf(file, "All Values (In-Order): ");
    printAllValues(file, tree);
    fprintf(file, "\n========================================\n");

    fclose(file);
}

void saveNodeToFile(FILE* file, BPlusNode* node, int depth) {
    if (!node || !file) return;

    for (int i = 0; i < depth; i++) {
        fprintf(file, "  ");
    }

    fprintf(file, "[%s] Keys: ", node->isLeaf ? "LEAF" : "INTERNAL");
    for (int i = 0; i < node->numKeys; i++) {
        fprintf(file, "%d", node->keys[i]);
        if (i < node->numKeys - 1) fprintf(file, ", ");
    }
    fprintf(file, "\n");

    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; i++) {
            if (node->children[i]) {
                saveNodeToFile(file, node->children[i], depth + 1);
            }
        }
    }
}

void printAllValues(FILE* file, BPlusTree* tree) {
    if (!tree || !tree->root) return;

    BPlusNode* leaf = tree->root;
    while (!leaf->isLeaf) {
        leaf = leaf->children[0];
    }

    while (leaf) {
        for (int i = 0; i < leaf->numKeys; i++) {
            fprintf(file, "%d ", leaf->keys[i]);
        }
        leaf = leaf->next;
    }
}

BPlusTree* loadTree(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return createTree();
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize < 50) {
        fclose(file);
        return createTree();
    }
    rewind(file);

    BPlusTree* tree = createTree();
    char line[1024];

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "All Values")) {
            char* ptr = strchr(line, ':');
            if (ptr) {
                ptr++;
                char* token = strtok(ptr, " ,\n\t");
                while (token != NULL) {
                    int value = atoi(token);
                    if (value != 0 || (token[0] == '0' && token[1] == '\0')) {
                        insert(tree, value);
                    }
                    token = strtok(NULL, " ,\n\t");
                }
            }
            break;
        }
    }

    fclose(file);
    return tree;
}

void freeTree(BPlusNode* node) {
    if (!node) return;

    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; i++) {
            if (node->children[i]) {
                freeTree(node->children[i]);
            }
        }
    }
    free(node);
}

void closeTree(BPlusTree* tree) {
    if (tree) {
        if (tree->root) {
            freeTree(tree->root);
        }
        free(tree);
    }
}

char* urlDecode(char* str) {
    char* decoded = str;
    char* pstr = str;

    while (*pstr) {
        if (*pstr == '%') {
            if (pstr[1] && pstr[2]) {
                int value;
                sscanf(pstr + 1, "%2x", &value);
                *decoded++ = (char)value;
                pstr += 3;
            }
        }
        else if (*pstr == '+') {
            *decoded++ = ' ';
            pstr++;
        }
        else {
            *decoded++ = *pstr++;
        }
    }
    *decoded = '\0';
    return str;
}

void parsePostData(char* data, char* action, int* value) {
    char* token = strtok(data, "&");

    while (token != NULL) {
        if (strncmp(token, "action=", 7) == 0) {
            strcpy(action, urlDecode(token + 7));
        }
        else if (strncmp(token, "value=", 6) == 0) {
            *value = atoi(urlDecode(token + 6));
        }
        token = strtok(NULL, "&");
    }
}

void generateHTMLTree(BPlusTree* tree, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) return;

    fprintf(file, "<!DOCTYPE html>\n<html>\n<head>\n");
    fprintf(file, "<meta charset='UTF-8'>\n");
    fprintf(file, "<title>B+ Tree Visualization</title>\n");
    fprintf(file, "<style>\n");
    fprintf(file, "body { font-family: Arial; background: linear-gradient(135deg, #667eea 0%%, #764ba2 100%%); min-height: 100vh; padding: 40px; }\n");
    fprintf(file, ".container { background: white; padding: 40px; border-radius: 20px; box-shadow: 0 20px 60px rgba(0,0,0,0.3); max-width: 900px; margin: 0 auto; }\n");
    fprintf(file, "h2 { text-align: center; color: #333; margin-bottom: 30px; }\n");
    fprintf(file, ".tree { margin: 40px 0; }\n");
    fprintf(file, ".level { display: flex; justify-content: center; margin: 30px 0; }\n");
    fprintf(file, ".node { background: white; border: 3px solid #333; border-radius: 10px; padding: 15px 20px; margin: 0 15px; box-shadow: 0 4px 8px rgba(0,0,0,0.2); min-width: 80px; text-align: center; }\n");
    fprintf(file, ".internal { background: #e3f2fd; border-color: #1976d2; }\n");
    fprintf(file, ".leaf { background: #f1f8e9; border-color: #689f38; }\n");
    fprintf(file, ".key { display: inline-block; padding: 8px 12px; margin: 3px; font-weight: bold; font-size: 18px; }\n");
    fprintf(file, ".values { margin-top: 40px; padding: 25px; background: #fff3e0; border-radius: 10px; text-align: center; border: 2px solid #ff9800; }\n");
    fprintf(file, ".values strong { color: #e65100; }\n");
    fprintf(file, ".value-item { display: inline-block; padding: 10px 15px; background: #4caf50; color: white; border-radius: 8px; margin: 5px; font-weight: bold; box-shadow: 0 2px 4px rgba(0,0,0,0.2); }\n");
    fprintf(file, ".back-btn { display: block; text-align: center; margin-top: 30px; }\n");
    fprintf(file, ".back-btn a { display: inline-block; padding: 12px 30px; background: linear-gradient(135deg, #667eea 0%%, #764ba2 100%%); color: white; text-decoration: none; border-radius: 8px; font-weight: 600; }\n");
    fprintf(file, "</style>\n");
    fprintf(file, "</head>\n<body>\n");
    fprintf(file, "<div class='container'>\n");
    fprintf(file, "<h2>🌳 B+ Tree Visualization (Order %d)</h2>\n", ORDER);

    if (tree && tree->root) {
        fprintf(file, "<div class='tree'>\n");
        printNodeHTML(file, tree->root, 0);
        fprintf(file, "</div>\n");

        fprintf(file, "<div class='values'>\n");
        fprintf(file, "<strong>📊 All Values (Sorted): </strong><br><br>\n");
        printAllValuesHTML(file, tree);
        fprintf(file, "</div>\n");
    }
    else {
        fprintf(file, "<p style='text-align:center; color:#999;'>Tree is empty</p>\n");
    }

    fprintf(file, "<div class='back-btn'><a href='/bplustree/index.html'>← Back to Manager</a></div>\n");
    fprintf(file, "</div>\n");
    fprintf(file, "</body>\n</html>\n");
    fclose(file);
}

void printNodeHTML(FILE* file, BPlusNode* node, int level) {
    if (!node) return;

    static int needClosing = 0;

    if (level == 0) {
        fprintf(file, "<div class='level'>\n");
        needClosing = 1;
    }

    fprintf(file, "<div class='node %s'>\n", node->isLeaf ? "leaf" : "internal");
    fprintf(file, "<div style='font-size:11px; color:#999; margin-bottom:5px;'>%s</div>\n",
        node->isLeaf ? "[LEAF]" : "[INTERNAL]");

    for (int i = 0; i < node->numKeys; i++) {
        fprintf(file, "<span class='key'>%d</span>", node->keys[i]);
        if (i < node->numKeys - 1) fprintf(file, " <span style='color:#ccc;'>|</span> ");
    }
    fprintf(file, "</div>\n");

    if (level == 0 && needClosing) {
        fprintf(file, "</div>\n");
        needClosing = 0;
    }

    if (!node->isLeaf) {
        fprintf(file, "<div class='level'>\n");
        for (int i = 0; i <= node->numKeys; i++) {
            if (node->children[i]) {
                fprintf(file, "<div class='node %s'>\n", node->children[i]->isLeaf ? "leaf" : "internal");
                fprintf(file, "<div style='font-size:11px; color:#999; margin-bottom:5px;'>%s</div>\n",
                    node->children[i]->isLeaf ? "[LEAF]" : "[INTERNAL]");

                for (int j = 0; j < node->children[i]->numKeys; j++) {
                    fprintf(file, "<span class='key'>%d</span>", node->children[i]->keys[j]);
                    if (j < node->children[i]->numKeys - 1) fprintf(file, " <span style='color:#ccc;'>|</span> ");
                }
                fprintf(file, "</div>\n");
            }
        }
        fprintf(file, "</div>\n");

        if (!node->children[0]->isLeaf) {
            for (int i = 0; i <= node->numKeys; i++) {
                if (node->children[i]) {
                    printNodeHTML(file, node->children[i], level + 1);
                }
            }
        }
    }
}

void printAllValuesHTML(FILE* file, BPlusTree* tree) {
    if (!tree || !tree->root) return;

    BPlusNode* leaf = tree->root;
    while (!leaf->isLeaf) {
        leaf = leaf->children[0];
    }

    while (leaf) {
        for (int i = 0; i < leaf->numKeys; i++) {
            fprintf(file, "<span class='value-item'>%d</span> ", leaf->keys[i]);
        }
        leaf = leaf->next;
    }
}

int main() {
    printf("Content-Type: text/html\r\n\r\n");

    char* lenstr = getenv("CONTENT_LENGTH");
    char* method = getenv("REQUEST_METHOD");

    if (!method || strcmp(method, "POST") != 0) {
        printf("<html><body><h2>Error: Only POST method supported</h2></body></html>");
        return 1;
    }

    int contentLength = 0;
    if (lenstr) {
        contentLength = atoi(lenstr);
    }

    if (contentLength <= 0 || contentLength > 10000) {
        printf("<html><body><h2>Error: Invalid content length</h2></body></html>");
        return 1;
    }

    char* postData = (char*)malloc(contentLength + 1);
    if (!postData) {
        printf("<html><body><h2>Error: Memory allocation failed</h2></body></html>");
        return 1;
    }

    fread(postData, 1, contentLength, stdin);
    postData[contentLength] = '\0';

    char action[20] = "";
    int value = 0;
    parsePostData(postData, action, &value);
    free(postData);

    BPlusTree* tree = loadTree("C:/xampp/htdocs/bplustree/bplustree.txt");
    if (!tree) {
        printf("<html><body><h2>Error: Could not create tree</h2></body></html>");
        return 1;
    }

    printf("<!DOCTYPE html>\n");
    printf("<html><head>\n");
    printf("<meta charset='UTF-8'>\n");
    printf("<style>\n");
    printf("body { font-family: 'Segoe UI', Tahoma, sans-serif; background: linear-gradient(135deg, #667eea 0%%, #764ba2 100%%); min-height: 100vh; display: flex; justify-content: center; align-items: center; margin: 0; padding: 20px; }\n");
    printf(".container { background: white; border-radius: 20px; padding: 40px; box-shadow: 0 20px 60px rgba(0,0,0,0.3); max-width: 500px; text-align: center; }\n");
    printf("h2 { color: #333; margin-bottom: 20px; }\n");
    printf(".success { color: #155724; background: #d4edda; padding: 15px; border-radius: 8px; margin: 20px 0; }\n");
    printf(".error { color: #721c24; background: #f8d7da; padding: 15px; border-radius: 8px; margin: 20px 0; }\n");
    printf("a { display: inline-block; margin: 10px 5px; padding: 12px 24px; background: linear-gradient(135deg, #667eea 0%%, #764ba2 100%%); color: white; text-decoration: none; border-radius: 8px; font-weight: 600; }\n");
    printf("a:hover { transform: translateY(-2px); box-shadow: 0 5px 15px rgba(0,0,0,0.2); }\n");
    printf("</style>\n");
    printf("</head><body>\n");
    printf("<div class='container'>\n");

    if (strcmp(action, "insert") == 0) {
        if (value == 0) {
            printf("<h2>⚠️ Invalid Input</h2>\n");
            printf("<div class='error'>Please enter a valid number. Zero is not allowed (or field was empty).</div>\n");
            printf("<a href='/bplustree/index.html'>Go Back</a>\n");
        }
        else {
            insert(tree, value);
            saveTree(tree, "C:/xampp/htdocs/bplustree/bplustree.txt");
            generateHTMLTree(tree, "C:/xampp/htdocs/bplustree/tree_visual.html");

            printf("<h2>✓ Success!</h2>\n");
            printf("<div class='success'>Successfully inserted <strong>%d</strong> into B+ Tree</div>\n", value);
            printf("<a href='/bplustree/index.html'>Insert Another</a>\n");
            printf("<a href='/bplustree/tree_visual.html' target='_blank'>🌳 View Visual Tree</a>\n");
        }
    }
    else if (strcmp(action, "search") == 0) {
        int found = search(tree, value);

        if (found) {
            printf("<h2>✓ Found!</h2>\n");
            printf("<div class='success'>Value <strong>%d</strong> exists in the B+ Tree</div>\n", value);
        }
        else {
            printf("<h2>✗ Not Found</h2>\n");
            printf("<div class='error'>Value <strong>%d</strong> does not exist in the B+ Tree</div>\n", value);
        }
        printf("<a href='/bplustree/index.html'>Go Back</a>\n");
        printf("<a href='/bplustree/tree_visual.html' target='_blank'>🌳 View Visual Tree</a>\n");
    }
    else if (strcmp(action, "reset") == 0) {
        remove("C:/xampp/htdocs/bplustree/bplustree.txt");
        remove("C:/xampp/htdocs/bplustree/tree_visual.html");

        printf("<h2>🗑️ Reset Complete!</h2>\n");
        printf("<div class='success'>All data has been deleted successfully.<br>The tree is now empty and ready for new data.</div>\n");
        printf("<a href='/bplustree/index.html'>Start Fresh</a>\n");
    }

    printf("</div>\n");
    printf("</body></html>\n");

    closeTree(tree);
    return 0;
}