#!/bin/bash

# fix_tank_z <fromdate> <threshold>
# example of use
#  ./delete_from_tankz.sh 14-May-2024 850

DATE=$1
THRESHOLD=$2

function delete_row {
    local nanoseconds=$1
    curl -G 'http://localhost:8086/query?pretty=true' --data-urlencode "db=devices" --data-urlencode "q=DELETE  FROM \"tank.Z\" WHERE time=$nanoseconds"
}

function clean_data {
    nanoseconds="$(date -d "$DATE" +'%s')000000000"
    curl -s 'http://localhost:8086/query?db=devices' --data-urlencode "q=SELECT * FROM \"tank.Z\" WHERE time > $nanoseconds AND value > $THRESHOLD" | jq -r '.results[0].series[0].values[] | .[0]' > timestamps
    while read timestamp; do
        seconds=$(date -d "$timestamp" +%s)
	nanoseconds=$((seconds * 1000000000))
	delete_row $nanoseconds
    done < timestamps
}

clean_data

