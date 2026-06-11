import os
import sys
import subprocess
import tarfile
import shutil
import time
from flask import Flask, render_template, request, send_file, after_this_request

app = Flask(__name__)
BASE_DIR = os.path.abspath(os.path.dirname(__file__))
UPLOAD_FOLDER = os.path.join(BASE_DIR, 'uploads')
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

# OS AGNOSTIC PATHS - FORCE ABSOLUTE PATHS
is_windows = sys.platform.startswith('win')
COMPRESSOR_EXE = os.path.join(BASE_DIR, 'compressorexp.exe' if is_windows else 'compressorexp')
DECOMPRESSOR_EXE = os.path.join(BASE_DIR, 'decompressorexp.exe' if is_windows else 'decompressorexp')

@app.route('/')
def index():
    return render_template('indexexp.html')

@app.route('/compress', methods=['POST'])
def compress_file():
    password = request.form.get('password', '')
    files = request.files.getlist('file')
    if not files: return "No files uploaded", 400

    os.makedirs(UPLOAD_FOLDER, exist_ok=True)

    if len(files) > 1:
        # BATCH MODE
        tar_name = "batch_bundle.tar"
        tar_path = os.path.join(UPLOAD_FOLDER, tar_name)
        try:
            with tarfile.open(tar_path, "w") as tar:
                for f in files:
                    safe_filename = f.filename.replace(" ", "_")
                    save_p = os.path.join(UPLOAD_FOLDER, safe_filename)
                    f.save(save_p)
                    tar.add(save_p, arcname=safe_filename)
                    os.remove(save_p)
        except Exception as e:
            return f"Batching Failed: {str(e)}", 500

        input_path = tar_path
        output_filename = "compressed_batch_bundle.huff"
    else:
        # SINGLE FILE
        f = files[0]
        safe_filename = f.filename.replace(" ", "_")
        input_path = os.path.join(UPLOAD_FOLDER, safe_filename)
        f.save(input_path)
        output_filename = f"compressed_{safe_filename}.huff"

    output_path = os.path.join(UPLOAD_FOLDER, output_filename)
    
    cmd = [os.path.abspath(COMPRESSOR_EXE), os.path.abspath(input_path), os.path.abspath(output_path)]
    if password: cmd.append(password)

    print(f"DEBUG: Running Compression: {cmd}", flush=True)

    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        return f"Compression Failed (C Error Code {e.returncode})", 500

    json_path = output_path + ".json"
    tree_data = '{"mode": "Unknown", "root": null}'
    if os.path.exists(json_path) and os.path.getsize(json_path) < 81920:
        try:
            with open(json_path, 'r') as jf: tree_data = jf.read()
        except: pass

    @after_this_request
    def cleanup(response):
        try:
            if os.path.exists(input_path): os.remove(input_path)
            if os.path.exists(output_path): os.remove(output_path)
            if os.path.exists(json_path): os.remove(json_path)
        except: pass
        return response

    res = send_file(output_path, as_attachment=True, download_name=output_filename)
    res.headers["X-Tree-Data"] = tree_data
    return res

@app.route('/decompress', methods=['POST'])
def decompress_file():
    password = request.form.get('password', '')
    if 'file' not in request.files: return "No file uploaded", 400
    
    f = request.files['file']
    filename = f.filename.replace(" ", "_")
    input_path = os.path.join(UPLOAD_FOLDER, filename)
    f.save(input_path)

    print(f"DEBUG: Saved upload to {input_path}", flush=True)

    abs_exe = os.path.abspath(DECOMPRESSOR_EXE)
    abs_input = os.path.abspath(input_path)
    
    cmd = [abs_exe, abs_input]
    if password: cmd.append(password)

    try:
        print(f"DEBUG: Executing C Decompressor: {cmd}", flush=True)
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        
        restored_path = res.stdout.strip().splitlines()[-1] if res.stdout else ""
        print(f"DEBUG: C Output Path: '{restored_path}'", flush=True)

    except subprocess.CalledProcessError as e:
        print(f"ERROR: C Decompressor Crashed! Stderr: {e.stderr}", flush=True)
        return "Decompression Failed (Wrong Password or File Corrupt)", 500

    if not restored_path or not os.path.exists(restored_path):
        base = os.path.basename(restored_path)
        fallback = os.path.join(UPLOAD_FOLDER, base)
        if os.path.exists(fallback): restored_path = fallback
        else: return f"Error: File '{restored_path}' not found on disk.", 500

    final_path = restored_path
    download_name = os.path.basename(restored_path)

    # --- BATCH HANDLING ---
    if restored_path.endswith('.tar'):
        print("DEBUG: Batch TAR detected. Processing...", flush=True)
        time.sleep(1.0) # Wait for Windows file lock

        extract_dir = os.path.join(UPLOAD_FOLDER, f"temp_{int(time.time())}")
        os.makedirs(extract_dir, exist_ok=True)

        try:
            with tarfile.open(restored_path, "r") as tar:
                tar.extractall(path=extract_dir)
            
            zip_base = os.path.join(UPLOAD_FOLDER, "restored_batch")
            shutil.make_archive(zip_base, 'zip', extract_dir)
            
            final_path = zip_base + ".zip"
            download_name = "restored_files.zip"
            
            try:
                os.remove(restored_path)
                shutil.rmtree(extract_dir)
            except: pass

        except Exception as e:
            print(f"ERROR during TAR extraction: {e}", flush=True)
            return f"Batch Extraction Error: {e}", 500

    @after_this_request
    def cleanup(response):
        try: 
            if os.path.exists(input_path): os.remove(input_path)
            if os.path.exists(final_path): os.remove(final_path)
        except: pass
        return response

    return send_file(final_path, as_attachment=True, download_name=download_name)

if __name__ == '__main__':
    # CRITICAL FIX: use_reloader=False prevents server restart loops
    app.run(debug=True, use_reloader=False, host='0.0.0.0', port=5001, threaded=True)