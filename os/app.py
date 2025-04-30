from flask import Flask, render_template, jsonify
import time

def read_data():
    try:
        with open("data.txt", "r") as file:
            lines = file.readlines()
            cpu_usage = []
            mem_usage = []
            for line in lines[-50:]:  # Garder seulement les 50 dernières entrées
                parts = line.strip().split()
                if len(parts) == 2:
                    cpu_usage.append(int(parts[0]))
                    mem_usage.append(int(parts[1]))
            return cpu_usage, mem_usage
    except FileNotFoundError:
        return [], []

app = Flask(__name__)

@app.route('/')
def index():
    return open("templates/index.html").read()

@app.route('/data')
def data():
    cpu_usage, mem_usage = read_data()
    return jsonify({"cpu": cpu_usage, "memory": mem_usage})

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
