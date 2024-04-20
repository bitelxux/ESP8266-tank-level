#!/usr/bin/env python3

from datetime import datetime
from flask import Flask, Markup, render_template

import logging

log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

logfile = "/water_tank/robotits.log"

app = Flask(__name__)

@app.route('/')
def main():
     return "I'm alive!"

@app.route('/log/<string:msg>')
def log(msg):
    print("received: %s", msg)
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
    msg = f"{timestamp} {msg}"

    with open(logfile, "a") as f:
        f.write(f"{msg}\n")

    return "OK"

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8888, threaded=True)

