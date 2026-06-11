import random

def generate_sample(filename, size_mb):
    with open(filename, 'w', encoding='utf-8') as f:
        # Part 1: Highly repetitive strings for RLE
        for _ in range(size_mb * 5000):
            f.write("A" * 100 + "B" * 50 + "C" * 25 + "123" * 10 + "\n")
        
        # Part 2: Natural-like text for Huffman
        words = ["compression", "lossless", "algorithm", "data", "structure", "huffman", "tree", "encoding", "decoding", "flask", "C", "performance", "optimization", "system", "file", "archive"]
        for _ in range(size_mb * 10000):
            sentence = " ".join(random.choices(words, k=15)) + ".\n"
            f.write(sentence)

        # Part 3: Random alphanumeric for entropy
        for _ in range(size_mb * 2000):
            f.write("".join(random.choices("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", k=100)) + "\n")

if __name__ == "__main__":
    generate_sample("sample_input.txt", 1)  # ~1 MB of data
