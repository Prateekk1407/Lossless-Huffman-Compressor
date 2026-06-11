# Lossless Huffman Compressor

This project implements a lossless file compressor and decompressor using Huffman Coding and Run-Length Encoding (RLE). It also features an optional TEA (Tiny Encryption Algorithm) encryption layer to secure the compressed files. The backend compression logic is written in C for high performance, and it is wrapped in a Flask web application for an easy-to-use user interface.

## Features

- **Hybrid Compression**: Uses a combination of RLE and Huffman Coding depending on the file type and size to achieve optimal compression.
- **Encryption**: Optional TEA (Tiny Encryption Algorithm) encryption with a user-provided password.
- **Batch Processing**: Supports uploading and compressing multiple files at once.
- **Web Interface**: A Flask-based web application provides an intuitive way to interact with the compressor.
- **Performance**: High-performance C implementation handles large files efficiently.

## Prerequisites

- Python 3.x
- Flask
- GCC (or any standard C compiler to compile the source files)

## Setup

1. **Compile the C Code** (Optional if binaries are provided):
   ```bash
   gcc compressorexp.c -o compressorexp
   gcc decompressorexp.c -o decompressorexp
   ```

2. **Install Python Dependencies**:
   ```bash
   pip install flask
   ```

3. **Run the Application**:
   ```bash
   python appexp.py
   ```
   The application will be accessible at `http://localhost:5001`.

## Usage

1. Open the web interface in your browser.
2. Select one or more files to compress.
3. (Optional) Enter a password to encrypt the compressed archive.
4. Click "Compress" to download the `.huff` file.
5. To decompress, upload the `.huff` file, enter the password (if used), and click "Decompress".

## Sample Files

- `sample_input.txt`: A sample text file to test the compressor.
- `compressed_sample_input.txt.huff`: The resulting compressed (and potentially encrypted) file.
