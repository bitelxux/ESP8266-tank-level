#!/usr/bin/python3

import os
import statistics as stats

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

def is_outlier(new_value, timestamp):
    result = client.query(f"SELECT * FROM readings WHERE time <= {timestamp} ORDER BY time DESC LIMIT 1")
    readings = list(result.get_points(measurement='readings'))
    values =  [reading['value'] for reading in readings]

    return values && abs(new_value - values[0]) > 150:

@app.route('/add/<string:value>')
def add(value):

    id = request.headers.get('device_id')
    print(f"adding {id}:{value}")
    try:
        timestamp, value = value.split(":")
        int_timestamp = int(timestamp) * 1000000000
        int_value = int(value)

        if is_outlier(int_value, int_timestamp):
            print(f"{int_value} seems to be an outlier")
            return(f"{int_value} seems to be an outlier")

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
