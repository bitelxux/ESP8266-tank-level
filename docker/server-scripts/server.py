#!/usr/bin/python3

import os

from influxdb import InfluxDBClient
from flask import Flask, Markup, render_template, abort, request

client = InfluxDBClient(host='influxdb', port=8086, username='admin', password='admin')
client.switch_database('tank')

app = Flask(__name__)

@app.route('/')
def main():
     abort(403)

@app.route('/ping')
def ping():
     return "pong"

@app.route('/add/<string:value>')
def add(value):
    id = request.headers.get('device_id')
    print(f"adding {id}:{value}")
    try:
        timestamp, value = value.split(":")
        int_timestamp = int(timestamp) * 1000000000
        int_value = int(value)
        body = [
          {
              "measurement": "readings",
              "time": int_timestamp,
              "fields": {
                  "device_id": id,
                  "value": int_value
              }
          }
        ]
        client.write_points(body)
        return f"The value {value} has been gladly added"
    except:
        return f"Error adding {value}"

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8889, threaded=True)

