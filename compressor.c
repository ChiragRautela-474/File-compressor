#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CHAR 256

// ------------------ HUFFMAN STRUCTURES -------------------
typedef struct Node {
    char ch;
    int freq;
    struct Node *left, *right;
} Node;

Node* heap[MAX_CHAR];
int heapSize = 0;

// ------------------ HEAP FUNCTIONS -----------------------
void swap(Node **a, Node **b) {
    Node *t = *a;
    *a = *b;
    *b = t;
}

void insertHeap(Node *node) {
    heap[heapSize++] = node;
    int i = heapSize - 1;
    while (i && heap[i]->freq < heap[(i - 1) / 2]->freq) {
        swap(&heap[i], &heap[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

Node* removeMin() {
    Node* root = heap[0];
    heap[0] = heap[--heapSize];
    int i = 0, smallest;
    while (1) {
        int left = 2 * i + 1, right = 2 * i + 2;
        smallest = i;
        if (left < heapSize && heap[left]->freq < heap[smallest]->freq)
            smallest = left;
        if (right < heapSize && heap[right]->freq < heap[smallest]->freq)
            smallest = right;
        if (smallest == i) break;
        swap(&heap[i], &heap[smallest]);
        i = smallest;
    }
    return root;
}

Node* createNode(char ch, int freq) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->ch = ch;
    node->freq = freq;
    node->left = node->right = NULL;
    return node;
}

void buildCodes(Node *root, char *code, int depth, char *codes[MAX_CHAR]) {
    if (!root) return;
    if (!root->left && !root->right) {
        code[depth] = '\0';
        codes[(unsigned char)root->ch] = strdup(code);
        return;
    }
    code[depth] = '0';
    buildCodes(root->left, code, depth + 1, codes);
    code[depth] = '1';
    buildCodes(root->right, code, depth + 1, codes);
}

Node* rebuildTreeFromCodes(char *codes[MAX_CHAR]) {
    Node *root = createNode('$', 0);
    for (int i = 0; i < MAX_CHAR; i++) {
        if (codes[i]) {
            Node *curr = root;
            for (int j = 0; codes[i][j]; j++) {
                if (codes[i][j] == '0') {
                    if (!curr->left) curr->left = createNode('$', 0);
                    curr = curr->left;
                } else {
                    if (!curr->right) curr->right = createNode('$', 0);
                    curr = curr->right;
                }
            }
            curr->ch = (char)i;
        }
    }
    return root;
}

// ------------------ HUFFMAN COMPRESSION -------------------
void compressFile(const char *path) {
    FILE *in = fopen(path, "rb");
    if (!in) {
        perror("Failed to open file");
        return;
    }

    int freq[MAX_CHAR] = {0};
    int ch;
    while ((ch = fgetc(in)) != EOF) freq[ch]++;
    rewind(in);

    for (int i = 0; i < MAX_CHAR; i++) {
        if (freq[i]) insertHeap(createNode((char)i, freq[i]));
    }

    while (heapSize > 1) {
        Node *l = removeMin();
        Node *r = removeMin();
        Node *p = createNode('$', l->freq + r->freq);
        p->left = l; p->right = r;
        insertHeap(p);
    }

    Node *root = removeMin();
    char *codes[MAX_CHAR] = {0};
    char code[100];
    buildCodes(root, code, 0, codes);

    char outPath[300];
    snprintf(outPath, sizeof(outPath), "%s.huff", path);
    FILE *out = fopen(outPath, "w");
    if (!out) {
        perror("Failed to create output file");
        fclose(in);
        return;
    }

    for (int i = 0; i < MAX_CHAR; i++) {
        if (codes[i]) {
            unsigned char c = (unsigned char)i;
            if (c == '\n') fprintf(out, "\\n:%s\n", codes[i]);
            else if (c == '\r') fprintf(out, "\\r:%s\n", codes[i]);
            else if (c == '\t') fprintf(out, "\\t:%s\n", codes[i]);
            else if (c >= 32 && c <= 126)
                fprintf(out, "%c:%s\n", c, codes[i]);
            else
                fprintf(out, "\\x%02X:%s\n", c, codes[i]);
        }
    }

    fprintf(out, "\n");

    while ((ch = fgetc(in)) != EOF) fputs(codes[ch], out);

    printf("Compressed file saved as: %s\n", outPath);

    fclose(in);
    fclose(out);
    for (int i = 0; i < MAX_CHAR; i++) free(codes[i]);
}

// ------------------ HUFFMAN DECOMPRESSION -------------------
void decompressFile(const char *path) {
    FILE *in = fopen(path, "r");
    if (!in) {
        perror("Failed to open compressed file");
        return;
    }

    char line[300];
    char *codes[MAX_CHAR] = {0};

    while (fgets(line, sizeof(line), in)) {
        if (line[0] == '\n') break;

        char key[10];
        char code[256];

        if (sscanf(line, "%9[^:]:%255s", key, code) == 2) {
            unsigned char ascii_char;

            if (key[0] == '\\') {
                if (strcmp(key, "\\n") == 0) ascii_char = '\n';
                else if (strcmp(key, "\\r") == 0) ascii_char = '\r';
                else if (strcmp(key, "\\t") == 0) ascii_char = '\t';
                else if (strncmp(key, "\\x", 2) == 0) {
                    unsigned int val;
                    sscanf(key + 2, "%2x", &val);
                    ascii_char = (unsigned char)val;
                } else {
                    ascii_char = (unsigned char)key[1];
                }
            } else {
                ascii_char = (unsigned char)key[0];
            }

            codes[ascii_char] = strdup(code);
        }
    }

    Node *root = rebuildTreeFromCodes(codes);
    Node *curr = root;

    char ch;
    char outPath[300];
    snprintf(outPath, sizeof(outPath), "%s.dec", path);
    FILE *out = fopen(outPath, "w");
    if (!out) {
        perror("Failed to create output file");
        fclose(in);
        return;
    }

    while ((ch = fgetc(in)) != EOF) {
        if (ch == '0') curr = curr->left;
        else if (ch == '1') curr = curr->right;
        if (curr && !curr->left && !curr->right) {
            fputc(curr->ch, out);
            curr = root;
        }
    }

    printf("Decompressed file saved as: %s\n", outPath);

    fclose(in);
    fclose(out);
    for (int i = 0; i < MAX_CHAR; i++) free(codes[i]);
}

// ------------------ RLE COMPRESSION -------------------
void compfunct(const char *inputFilePath, const char *outputFilePath) {
    FILE *inputFile = fopen(inputFilePath, "rb");
    if (inputFile == NULL) {
        perror("Error opening input file");
        return;
    }

    FILE *outputFile = fopen(outputFilePath, "w");
    if (outputFile == NULL) {
        perror("Error opening output file");
        fclose(inputFile);
        return;
    }

    int currentChar, previousChar = EOF;
    int count = 0;

    while ((currentChar = fgetc(inputFile)) != EOF) {
        if (currentChar == previousChar) {
            count++;
        } else {
            if (previousChar != EOF) {
                fprintf(outputFile, "%c%d", previousChar, count);
            }
            previousChar = currentChar;
            count = 1;
        }
    }
    if (previousChar != EOF) {
        fprintf(outputFile, "%c%d", previousChar, count);
    }

    fclose(inputFile);
    fclose(outputFile);
    printf("File compressed using RLE successfully.\n");
}

// ------------------ OPTIMIZATION -------------------
void optimizeFile(const char *inputPath) {
    FILE *in = fopen(inputPath, "r");
    if (!in) {
        perror("Failed to open input file");
        return;
    }

    char outputPath[300];
    snprintf(outputPath, sizeof(outputPath), "%s.optimized.txt", inputPath);
    FILE *out = fopen(outputPath, "w");
    if (!out) {
        perror("Failed to create output file");
        fclose(in);
        return;
    }

    char word[100], prev[100] = "";
    int ch, idx = 0;
    int spacePrinted = 0;

    while ((ch = fgetc(in)) != EOF) {
        if (isspace(ch)) {
            if (idx > 0) {
                word[idx] = '\0';
                if (strcmp(word, prev) != 0) {
                    if (ftell(out) > 0) fputc(' ', out);
                    fputs(word, out);
                    strcpy(prev, word);
                }
                idx = 0;
            }
            spacePrinted = 1;
        } else {
            word[idx++] = ch;
            spacePrinted = 0;
        }
    }

    if (idx > 0) {
        word[idx] = '\0';
        if (strcmp(word, prev) != 0) {
            if (ftell(out) > 0) fputc(' ', out);
            fputs(word, out);
        }
    }

    printf("Optimized file saved as: %s\n", outputPath);

    fclose(in);
    fclose(out);
}

// ------------------ MAIN -------------------
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: compressor <mode> <filepath>\n");
        return 1;
    }

    int mode = atoi(argv[1]);
    char *filePath = argv[2];
    char outputFilePath[300];

    if (mode == 1) {
        compressFile(filePath);
    } else if (mode == 2) {
        strcpy(outputFilePath, filePath);
        strcat(outputFilePath, ".modded.txt");
        compfunct(filePath, outputFilePath);
    } else if (mode == 3) {
        decompressFile(filePath);
    } else if (mode == 4) {
        optimizeFile(filePath);
    } else {
        printf("Invalid mode selected.\n");
        return 1;
    }

    return 0;
}

