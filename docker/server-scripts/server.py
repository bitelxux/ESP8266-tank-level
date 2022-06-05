#!/usr/bin/python3

import os
from flask import Flask, Markup, render_template, abort

app = Flask(__name__)

@app.route('/')
def main():
     abort(403)

@app.route('/ping')
def ping():
     return "pong"

@app.route('/add/<string:value>')
def add(value):
    print(f"adding {value}")
    return "The value %s has been gladly added" % value

@app.route('/todo')
def todo():
    try:
        task = open("todo").read()[:-1];
        os.system("rm -f todo")
    except:
        task = "nothing"

    return task

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8889, threaded=True)

