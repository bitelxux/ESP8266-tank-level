#!/usr/bin/python3

import os
import statistics as stats

from influxdb import InfluxDBClient
from flask import Flask, Markup, render_template, abort, request

SKIP_OUTLIER_DETECTION = "/water_tank/SKIP_OUTLIER_DETECTION"

client = InfluxDBClient(host='influxdb', port=8086, username='admin', password='admin')
client.switch_database('devices')

app = Flask(__name__)

def log(board, message):
    from datetime import datetime

    now = datetime.now()
    current_time = now.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

    with open("/water_tank/robotits.log", "a") as f:
        f.write(f"{current_time} [{board}] {message}\n")


@app.route('/')
def main():
     abort(403)

@app.route('/ping')
def ping():
     return "pong"

def is_outlier(device, new_value, timestamp):

    #TODO this is garbage. The whole outlier thing needs to be refactored to support different devices

    if device not in ['tank.Z']:
        return False

    if os.path.isfile(SKIP_OUTLIER_DETECTION):
        log('server', f"Outlier detection is disabled. Delete {SKIP_OUTLIER_DETECTION} to re-enable")
        return False

    if new_value in [960] or new_value > 1300:
        return True

    result = client.query(f"SELECT * FROM \"{device}\" WHERE time <= {timestamp} ORDER BY time DESC LIMIT 1")
    readings = list(result.get_points(measurement=f"{device}"))
    values =  [reading['value'] for reading in readings]

    outlier = values and abs(new_value - values[0]) > 200

    if outlier:
        log(device, f"{new_value} is an outlier")
    else:
        log(device, f"{new_value} is NOT an outlier")

    return outlier

@app.route('/getSum/<string:id>')
def getSum(id):
    result = client.query(f"SELECT SUM(value) FROM {id} LIMIT 1")
    # result[id] is a generator. In this case with only one value
    for i in result[id]:
      return(str(i['sum']))

@app.route('/add/<string:value>')
def add(value):

    id = request.headers.get('device_id')
    print(f"adding {id}:{value}")
    try:
        timestamp, value = value.split(":")
        int_timestamp = int(timestamp) * 1000000000
        int_value = int(value)

        if is_outlier(id, int_value, int_timestamp):
            print(f"{int_value} seems to be an outlier")
            return(f"{int_value} seems to be an outlier")

        body = [
          {
              "measurement": id,
              "time": int_timestamp,
              "fields": {
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
