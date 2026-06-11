#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define CHUNK_SIZE (1024 * 1024)
#define OUT_BUFFER_SIZE (32 * 1024)

// --- TEA ENCRYPTION MODULE ---
uint32_t key[4] = {0};
int is_encrypted = 0;

void setup_key(char *pwd) {
    if (!pwd || strlen(pwd) == 0) return;
    is_encrypted = 1;
    unsigned char k_bytes[16] = {0};
    strncpy((char*)k_bytes, pwd, 16);
    memcpy(key, k_bytes, 16);
}

void tea_encrypt_block(uint32_t* v) {
    uint32_t v0 = v[0], v1 = v[1], sum = 0, i;
    uint32_t delta = 0x9e3779b9;
    uint32_t k0 = key[0], k1 = key[1], k2 = key[2], k3 = key[3];
    for (i = 0; i < 32; i++) {
        sum += delta;
        v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
    }
    v[0] = v0; v[1] = v1;
}

void process_buffer_tea(unsigned char* buffer, size_t len) {
    if (!is_encrypted) return;
    size_t blocks = len / 8;
    for (size_t i = 0; i < blocks; i++) {
        tea_encrypt_block((uint32_t*)(buffer + (i * 8)));
    }
    // XOR remaining bytes
    for (size_t i = blocks * 8; i < len; i++) {
        buffer[i] ^= ((unsigned char*)key)[i % 16];
    }
}

// --- HUFFMAN STRUCTURES ---
struct MinHeapNode { char data; unsigned freq; struct MinHeapNode *left, *right; };
struct MinHeap { unsigned size; unsigned capacity; struct MinHeapNode** array; };

struct MinHeapNode* newNode(char data, unsigned freq) {
    struct MinHeapNode* temp = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));
    temp->left = temp->right = NULL; temp->data = data; temp->freq = freq; return temp;
}
struct MinHeap* createMinHeap(unsigned capacity) {
    struct MinHeap* minHeap = (struct MinHeap*)malloc(sizeof(struct MinHeap));
    minHeap->size = 0; minHeap->capacity = capacity;
    minHeap->array = (struct MinHeapNode**)malloc(capacity * sizeof(struct MinHeapNode*)); return minHeap;
}
void swapMinHeapNode(struct MinHeapNode** a, struct MinHeapNode** b) { struct MinHeapNode* t = *a; *a = *b; *b = t; }
void minHeapify(struct MinHeap* minHeap, int idx) {
    int smallest = idx, left = 2 * idx + 1, right = 2 * idx + 2;
    if (left < (int)minHeap->size && minHeap->array[left]->freq < minHeap->array[smallest]->freq) smallest = left;
    if (right < (int)minHeap->size && minHeap->array[right]->freq < minHeap->array[smallest]->freq) smallest = right;
    if (smallest != idx) { swapMinHeapNode(&minHeap->array[smallest], &minHeap->array[idx]); minHeapify(minHeap, smallest); }
}
struct MinHeapNode* extractMin(struct MinHeap* minHeap) {
    struct MinHeapNode* temp = minHeap->array[0];
    minHeap->array[0] = minHeap->array[minHeap->size - 1]; --minHeap->size; minHeapify(minHeap, 0); return temp;
}
void insertMinHeap(struct MinHeap* minHeap, struct MinHeapNode* node) {
    int i = minHeap->size++;
    while (i && node->freq < minHeap->array[(i - 1) / 2]->freq) { minHeap->array[i] = minHeap->array[(i - 1) / 2]; i = (i - 1) / 2; }
    minHeap->array[i] = node;
}
struct MinHeapNode* buildHuffmanTree(char data[], int freq[], int size) {
    struct MinHeapNode *left, *right, *top; struct MinHeap* minHeap = createMinHeap(size);
    for (int i = 0; i < size; i++) minHeap->array[i] = newNode(data[i], freq[i]);
    minHeap->size = size;
    for (int i = (size - 1) / 2; i >= 0; i--) minHeapify(minHeap, i);
    while (minHeap->size > 1) {
        left = extractMin(minHeap); right = extractMin(minHeap);
        top = newNode('$', left->freq + right->freq); top->left = left; top->right = right; insertMinHeap(minHeap, top);
    }
    return extractMin(minHeap);
}
void printTreeJSONRecursive(struct MinHeapNode* root, FILE* fp) {
    if (!root) return;
    fprintf(fp, "{\"name\": ");
    if (!root->left && !root->right) {
        unsigned char c = (unsigned char)root->data;
        if(c >= 32 && c <= 126 && c != '"' && c != '\\') fprintf(fp, "\"%c\"", c);
        else fprintf(fp, "\"0x%02X\"", c);
    } else { fprintf(fp, "\"%d\"", root->freq); }
    fprintf(fp, ", \"children\": [");
    if (root->left) printTreeJSONRecursive(root->left, fp);
    if (root->left && root->right) fprintf(fp, ",");
    if (root->right) printTreeJSONRecursive(root->right, fp);
    fprintf(fp, "]}");
}

char codes[256][256];
char path[256];
int top = 0;
void generateCodes(struct MinHeapNode* root) {
    if (root->left) { path[top++] = '0'; generateCodes(root->left); top--; }
    if (root->right) { path[top++] = '1'; generateCodes(root->right); top--; }
    if (!root->left && !root->right) { path[top] = '\0'; strcpy(codes[(unsigned char)root->data], path); }
}

unsigned char outBuffer[OUT_BUFFER_SIZE];
int outBufIndex = 0;
unsigned char bitBuffer = 0;
int bitCount = 0;

void flush_output_buffer(FILE* out) {
    if (outBufIndex > 0) {
        process_buffer_tea(outBuffer, outBufIndex);
        fwrite(outBuffer, 1, outBufIndex, out);
        outBufIndex = 0;
    }
}
void writeBit(int bit, FILE* out) {
    if (bit == 1) bitBuffer |= (1 << (7 - bitCount));
    bitCount++;
    if (bitCount == 8) {
        outBuffer[outBufIndex++] = bitBuffer;
        if (outBufIndex == OUT_BUFFER_SIZE) flush_output_buffer(out);
        bitBuffer = 0; bitCount = 0;
    }
}
void finish_writing(FILE* out) {
    if (bitCount > 0) outBuffer[outBufIndex++] = bitBuffer;
    flush_output_buffer(out);
}
unsigned char* rle_compress(unsigned char* input, long inputSize, long* outSize) {
    unsigned char* output = (unsigned char*)malloc(inputSize * 2 + 1);
    if (!output) return NULL;
    long outIndex = 0;
    for (long i = 0; i < inputSize; i++) {
        unsigned char count = 1;
        while (i < inputSize - 1 && input[i] == input[i + 1] && count < 255) { count++; i++; }
        output[outIndex++] = count; output[outIndex++] = input[i];
    }
    *outSize = outIndex;
    return output;
}
void get_clean_stored_name(char* fullPath, char* dest) {
    char *base = strrchr(fullPath, '\\');
    if (!base) base = strrchr(fullPath, '/');
    if (base) base++; else base = fullPath;
    strncpy(dest, base, 255);
    dest[255] = '\0';
    int len = strlen(dest);
    if (len > 5 && strcmp(dest + len - 5, ".huff") == 0) dest[len - 5] = '\0';
}

int main(int argc, char* argv[]) {
    if (argc < 3) return 1;
    FILE* in = fopen(argv[1], "rb");
    FILE* out = fopen(argv[2], "wb");
    if (!in || !out) return 1;

    if (argc == 4) { setup_key(argv[3]); }

    fseek(in, 0, SEEK_END);
    int64_t originalSize = ftell(in);
    rewind(in);

    int32_t useRLE = 0;
    int64_t sizeToCompress = originalSize;
    unsigned char* dataToCompress = NULL;
    int freq[256] = {0};
    struct MinHeapNode* root = NULL;

    if (originalSize > 50 * 1024 * 1024) { 
        unsigned char* sample = (unsigned char*)malloc(128 * 1024);
        size_t n = fread(sample, 1, 128*1024, in);
        rewind(in);
        int u=0; int f_s[256]={0}; for(size_t i=0;i<n;i++) f_s[sample[i]]++;
        for(int i=0;i<256;i++) if(f_s[i]) u++;
        free(sample);
        if (u > 250) useRLE = 3; 
        else {
            useRLE = 2; 
            unsigned char* chunk = (unsigned char*)malloc(CHUNK_SIZE);
            while ((n = fread(chunk, 1, CHUNK_SIZE, in)) > 0)
                for(size_t i=0; i<n; i++) freq[chunk[i]]++;
            free(chunk);
            rewind(in);
        }
    } else {
        unsigned char* rawData = (unsigned char*)malloc(originalSize);
        fread(rawData, 1, originalSize, in);
        long rleSize;
        unsigned char* rleData = rle_compress(rawData, originalSize, &rleSize);
        if (rleSize < originalSize) {
            useRLE = 1; dataToCompress = rleData; sizeToCompress = rleSize; free(rawData);
        } else {
            useRLE = 0; dataToCompress = rawData; sizeToCompress = originalSize; free(rleData);
        }
        for (long i = 0; i < sizeToCompress; i++) freq[dataToCompress[i]]++;
    }

    if (useRLE != 3) {
        char chars[256]; int f[256]; int uniqueSize = 0;
        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) { chars[uniqueSize] = (char)i; f[uniqueSize] = freq[i]; uniqueSize++; }
        }
        root = buildHuffmanTree(chars, f, uniqueSize);
        generateCodes(root);
    }

    char jsonPath[512]; sprintf(jsonPath, "%s.json", argv[2]);
    FILE* jsonFile = fopen(jsonPath, "w");
    if (jsonFile) {
        char* modeStr = (useRLE == 3)?"Stored (Video)":((useRLE==1)?"Hybrid RLE":"Standard");
        fprintf(jsonFile, "{ \"mode\": \"%s\", \"root\": ", modeStr);
        if(root) printTreeJSONRecursive(root, jsonFile); else fprintf(jsonFile, "null");
        fprintf(jsonFile, "}"); fclose(jsonFile);
    }

    char cleanName[256];
    get_clean_stored_name(argv[1], cleanName); 
    int32_t nameLen = strlen(cleanName);
    int32_t uniqueCount = 0; 
    for(int i=0; i<256; i++) if(freq[i] > 0) uniqueCount++;

    fwrite(&uniqueCount, sizeof(int32_t), 1, out);
    fwrite(&sizeToCompress, sizeof(int64_t), 1, out);
    fwrite(&useRLE, sizeof(int32_t), 1, out);
    fwrite(&nameLen, sizeof(int32_t), 1, out);
    
    char encryptedName[256];
    memset(encryptedName, 0, 256);
    memcpy(encryptedName, cleanName, nameLen);
    process_buffer_tea((unsigned char*)encryptedName, 256);
    fwrite(encryptedName, sizeof(char), 256, out);

    if (useRLE != 3) {
        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) { 
                unsigned char c = (unsigned char)i; 
                fwrite(&c, 1, 1, out); 
                int f_val = freq[i];
                process_buffer_tea((unsigned char*)&f_val, sizeof(int));
                fwrite(&f_val, sizeof(int), 1, out); 
            }
        }
    }

    if (useRLE == 3) {
        unsigned char* chunk = (unsigned char*)malloc(CHUNK_SIZE);
        size_t n;
        while ((n = fread(chunk, 1, CHUNK_SIZE, in)) > 0) {
            process_buffer_tea(chunk, n); 
            fwrite(chunk, 1, n, out);
        }
        free(chunk);
    } 
    else if (useRLE == 2) {
        unsigned char* chunk = (unsigned char*)malloc(CHUNK_SIZE);
        size_t n;
        while ((n = fread(chunk, 1, CHUNK_SIZE, in)) > 0) {
            for (size_t i = 0; i < n; i++) {
                char* code = codes[chunk[i]];
                for (int j = 0; code[j] != '\0'; j++) writeBit(code[j] - '0', out);
            }
        }
        free(chunk);
        finish_writing(out);
    } 
    else {
        for (long i = 0; i < sizeToCompress; i++) {
            char* code = codes[dataToCompress[i]];
            for (int j = 0; code[j] != '\0'; j++) writeBit(code[j] - '0', out);
        }
        free(dataToCompress);
        finish_writing(out);
    }
    
    fclose(in); fclose(out);
    return 0;
}