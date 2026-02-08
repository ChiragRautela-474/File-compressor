from flask import Flask, request, render_template, send_file
import subprocess
import os

app = Flask(__name__)
UPLOAD_FOLDER = 'uploads'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/compress', methods=['POST'])
def compress():
    mode = request.form['mode']
    file = request.files['file']
    safe_filename = file.filename.replace(" ", "_")
    filename = os.path.join(UPLOAD_FOLDER, safe_filename)
    file.save(filename)

    #  FIX: add ./ before compressor.exe
    result = subprocess.run(["./compressor.exe", mode, filename], capture_output=True, text=True)

    if mode == '1':
        output_file = filename + ".huff"
    elif mode == '2':
        output_file = filename + ".modded.txt"
    elif mode == '3':
        output_file = filename + ".dec"
    elif mode == '4':
        output_file = filename + ".optimized.txt"
    else:
        return "Invalid mode"

    if os.path.exists(output_file):
        return send_file(output_file, as_attachment=True, download_name=os.path.basename(output_file))
    else:
        return f"Error occurred: {result.stderr}"

if __name__ == '__main__':
    app.run(debug=True)
