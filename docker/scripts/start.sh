#!/bin/sh

/water_tank/server.py &
/water_tank/logserver.py &

tail -f /dev/null
