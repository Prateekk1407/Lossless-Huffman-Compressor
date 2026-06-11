# Lossless Huffman Compressor

An advanced, high-performance lossless file compressor and decompressor utility. This project implements a hybrid compression approach utilizing **Run-Length Encoding (RLE)** and **Huffman Coding**, tightly coupled with an optional **TEA (Tiny Encryption Algorithm)** layer for secure data archiving. 

The core compression and cryptographic operations are implemented in pure C to guarantee maximum performance and minimal memory footprint, while a Flask-based web application provides an intuitive graphical interface for end users.

---

## 🚀 Key Features

1. **Hybrid Compression Engine**
   - **Huffman Coding**: Builds an optimal prefix code based on the frequency of each byte in the file, significantly reducing the file size for files with varied byte distributions (like text and documents).
   - **Run-Length Encoding (RLE)**: Automatically detects when a file has long sequences of identical bytes and applies RLE to compress these blocks efficiently. The compressor dynamically chooses the best strategy (or a hybrid) based on an initial analysis of the input data.
   - **Large File Handling**: Automatically switches to memory-efficient chunking strategies for files over 50MB, preventing RAM overflow during processing.

2. **Integrated TEA Encryption**
   - **Security**: Compresses and encrypts your files in one fell swoop. The Tiny Encryption Algorithm is applied in a block-cipher mode, encrypting the data stream securely using a 128-bit key derived from your user password.
   - **Header Protection**: The original file metadata (such as the original filename) is also encrypted inside the `.huff` archive, preventing metadata leakage.

3. **Batch Archiving**
   - The frontend automatically detects when multiple files are uploaded, packs them into a TAR bundle, and compresses the entire bundle seamlessly. When decompressing, it automatically unpackages the TAR into a ZIP for easy access.

4. **Web Interface (Flask UI)**
   - A modern web UI that abstracts the command-line complexity. Upload files, provide passwords, and download `.huff` archives directly from your browser.
   - Generates a JSON representation of the Huffman Tree for educational visualization (for files under 80KB).

---

## 🏗️ Architecture

### 1. The Core Compressor (`compressorexp.c` / `decompressorexp.c`)
- **Min-Heap**: Used to build the Huffman Tree in `O(N log N)` time.
- **Bit-packing Buffer**: A 32KB output buffer minimizes disk I/O operations by packing bits tightly before flushing to disk.
- **`.huff` File Format Structure**:
  1. `uniqueCount` (4 bytes): Number of unique bytes in the file.
  2. `sizeToCompress` (8 bytes): Original uncompressed size of the data.
  3. `useRLE` (4 bytes): Flag indicating the compression mode used (Standard Huffman, Hybrid RLE, or Stored).
  4. `nameLen` (4 bytes): Length of the original file name.
  5. `encryptedName` (256 bytes): The original file name, encrypted via TEA.
  6. **Frequency Table**: Encrypted frequency data necessary to rebuild the Huffman tree during decompression.
  7. **Compressed Data Stream**: The bit-packed, encrypted Huffman/RLE payload.

### 2. The Web Wrapper (`appexp.py`)
- Acts as a bridge between the HTTP requests and the C executables using Python's `subprocess` module.
- Handles temporary file storage (`uploads/`), batching via the `tarfile` library, and cleanup using `@after_this_request` hooks.

---

## 🛠️ Prerequisites & Installation

To run this application locally, you will need:
- **Python 3.7+**
- **GCC** (GNU Compiler Collection) or any standard C compiler.

### 1. Clone and Compile
If the compiled binaries (`.exe` on Windows) are not present or you are on Linux/macOS, compile the C engines first:

```bash
git clone https://github.com/Prateekk1407/Lossless-Huffman-Compressor.git
cd Lossless-Huffman-Compressor

# Compile the compressor
gcc compressorexp.c -o compressorexp

# Compile the decompressor
gcc decompressorexp.c -o decompressorexp
```

### 2. Setup the Python Environment
Install the required Flask dependency. It is highly recommended to use a virtual environment.

```bash
pip install -r requirements.txt
# OR
pip install flask
```

### 3. Start the Server
Launch the Flask backend:
```bash
python appexp.py
```
The console will display `Running on http://0.0.0.0:5001/`. Open `http://localhost:5001` in your browser to access the GUI.

---

## 📖 How to Use

### Compressing Files
1. Navigate to the web application in your browser.
2. In the **Compress** tab, click **Choose Files**. You can select a single file or multiple files.
3. (Optional) Enter a **Password**. If provided, the archive will be encrypted and cannot be extracted without it.
4. Click **Compress**. The server will return a `.huff` file containing your compressed data.

### Decompressing Files
1. Navigate to the **Decompress** tab on the web application.
2. Upload the `.huff` file you previously generated.
3. If you used a password during compression, enter the exact same password.
4. Click **Decompress**. The server will extract and return your original file. (If you compressed a batch of files, it will return a `.zip` containing all original files).

---

## 📁 Sample Files Included

We have included a highly varied dummy dataset to demonstrate the efficiency of the hybrid compression engine:
- `sample_input.txt` (~3 MB): A generated text file containing blocks of highly repetitive patterns (ideal for RLE), randomized natural language sentences (ideal for Huffman Coding), and high-entropy alphanumeric strings.
- `sample_output.huff`: The compressed version of the sample input. Try decompressing it via the web app!

---

## ⚠️ Limitations & Notes
- **Video / Highly Compressed Files**: Files like `.mp4`, `.jpg`, or `.zip` are already compressed. Running them through Huffman compression will not yield a smaller file size (and might slightly increase it due to metadata headers). The compressor intelligently detects this and uses a "Stored" mode for extremely high-entropy data.
- **Passwords**: The system does not store passwords anywhere. If you lose the password to a `.huff` file, the data is unrecoverable. 

---
*Created by [Prateek Khemka](https://github.com/Prateekk1407).*
