#!/bin/sh

/server-scripts/server.py &
/server-scripts/logserver.py &

tail -f /dev/null
