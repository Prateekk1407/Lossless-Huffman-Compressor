#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define CHUNK_SIZE (32 * 1024) 

// --- TEA DECRYPTION MODULE ---
uint32_t key[4] = {0}; 
int is_encrypted = 0;

void setup_key(char *pwd) {
    if (!pwd || strlen(pwd) == 0) return;
    is_encrypted = 1;
    unsigned char k_bytes[16] = {0};
    strncpy((char*)k_bytes, pwd, 16);
    memcpy(key, k_bytes, 16);
}

void tea_decrypt_block(uint32_t* v) {
    uint32_t v0 = v[0], v1 = v[1], sum = 0xC6EF3720, i;
    uint32_t delta = 0x9e3779b9;
    uint32_t k0 = key[0], k1 = key[1], k2 = key[2], k3 = key[3];
    for (i = 0; i < 32; i++) {
        v1 -= ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
        v0 -= ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        sum -= delta;
    }
    v[0] = v0; v[1] = v1;
}

void process_buffer_tea_decrypt(unsigned char* buffer, size_t len) {
    if (!is_encrypted) return;
    size_t blocks = len / 8;
    for (size_t i = 0; i < blocks; i++) {
        tea_decrypt_block((uint32_t*)(buffer + (i * 8)));
    }
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

void generate_restored_name(char* headerName, char* finalPath, char* basePath) {
    char cleanName[256];
    strncpy(cleanName, headerName, 255);
    cleanName[255] = '\0';
    char *ext = strstr(cleanName, ".huff");
    if (ext) *ext = '\0';
    if (strncmp(cleanName, "compressed_", 11) == 0) memmove(cleanName, cleanName + 11, strlen(cleanName + 11) + 1);
    if (strncmp(cleanName, "restored_", 9) == 0) memmove(cleanName, cleanName + 9, strlen(cleanName + 9) + 1);
    char *lastSlash = strrchr(basePath, '\\'); 
    if (!lastSlash) lastSlash = strrchr(basePath, '/');
    if (lastSlash) {
        int dirLen = (int)(lastSlash - basePath + 1);
        strncpy(finalPath, basePath, dirLen); finalPath[dirLen] = '\0';
        strcat(finalPath, "restored_"); strcat(finalPath, cleanName);
    } else { sprintf(finalPath, "restored_%s", cleanName); }
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    FILE* in = fopen(argv[1], "rb");
    if (!in) return 1;

    if (argc == 3) { setup_key(argv[2]); }

    int32_t uniqueCount, useRLE, nameLen;
    int64_t totalChars;
    
    if (fread(&uniqueCount, sizeof(int32_t), 1, in) != 1) return 1;
    if (fread(&totalChars, sizeof(int64_t), 1, in) != 1) return 1;
    if (fread(&useRLE, sizeof(int32_t), 1, in) != 1) return 1;
    if (fread(&nameLen, sizeof(int32_t), 1, in) != 1) return 1;

    if (nameLen <= 0 || nameLen > 255) nameLen = 12;

    char originalName[256];
    if (fread(originalName, sizeof(char), 256, in) != 256) strcpy(originalName, "file.bin");
    else {
        process_buffer_tea_decrypt((unsigned char*)originalName, 256);
        originalName[nameLen] = '\0'; 
    }
    
    char outputPath[512];
    generate_restored_name(originalName, outputPath, argv[1]);

    FILE* out = fopen(outputPath, "wb");
    if (!out) { fclose(in); return 1; }

    if (useRLE == 3) {
        unsigned char* buffer = (unsigned char*)malloc(CHUNK_SIZE);
        size_t n;
        while ((n = fread(buffer, 1, CHUNK_SIZE, in)) > 0) {
            process_buffer_tea_decrypt(buffer, n); 
            fwrite(buffer, 1, n, out);
        }
        free(buffer); fclose(in); fclose(out);
        printf("%s", outputPath);
        return 0;
    }

    char chars[256]; int freq[256];
    for (int i = 0; i < uniqueCount; i++) {
        unsigned char c; 
        if(fread(&c, 1, 1, in) != 1) break;
        chars[i] = (char)c;
        int f_val;
        if(fread(&f_val, sizeof(int), 1, in) != 1) break;
        process_buffer_tea_decrypt((unsigned char*)&f_val, sizeof(int));
        freq[i] = f_val;
    }

    struct MinHeapNode* root = buildHuffmanTree(chars, freq, uniqueCount);
    struct MinHeapNode* curr = root;

    unsigned char* inBuf = (unsigned char*)malloc(CHUNK_SIZE);
    unsigned char* outBuf = (unsigned char*)malloc(CHUNK_SIZE);
    int outIdx = 0;
    long decodedCount = 0;
    int rleState = 0; unsigned char rleCount = 0;
    size_t n;

    while ((n = fread(inBuf, 1, CHUNK_SIZE, in)) > 0 && decodedCount < totalChars) {
        process_buffer_tea_decrypt(inBuf, n); 

        for (size_t i = 0; i < n; i++) {
            unsigned char byte = inBuf[i];
            for (int k = 7; k >= 0; k--) {
                int bit = (byte >> k) & 1;
                curr = (bit == 0) ? curr->left : curr->right;

                if (!curr->left && !curr->right) {
                    unsigned char val = (unsigned char)curr->data;
                    if (useRLE == 1) { // RLE
                        if (rleState == 0) { rleCount = val; rleState = 1; }
                        else { 
                            for(int j=0; j<rleCount; j++) {
                                outBuf[outIdx++] = val;
                                if(outIdx == CHUNK_SIZE) { fwrite(outBuf, 1, CHUNK_SIZE, out); outIdx = 0; }
                            }
                            rleState = 0; 
                        }
                    } else { // Standard
                        outBuf[outIdx++] = val;
                        if(outIdx == CHUNK_SIZE) { fwrite(outBuf, 1, CHUNK_SIZE, out); outIdx = 0; }
                    }
                    decodedCount++;
                    curr = root;
                    if (decodedCount == totalChars) break;
                }
            }
            if (decodedCount == totalChars) break;
        }
    }
    if (outIdx > 0) fwrite(outBuf, 1, outIdx, out);
    
    free(inBuf); free(outBuf);
    fclose(in); fclose(out);
    
    printf("%s", outputPath);
    return 0;
}