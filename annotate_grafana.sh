#!/bin/bash

# examples of use
# ./anotation.sh "11-Sep-2022 20:30:00.000" "this is a note"
# ./anotation.sh "11-Sep-2022 20:30"

DATE="$1"
TEXT=${2:-'NA'}
timestamp=$(date -d "$DATE" +'%s000')

function gen_body(){
  cat <<EOF
{
  "dashboardUID":"LlUpHMggk",
  "panelId":2,
  "time":$timestamp,
  "text":"$TEXT"
}
EOF
}

curl -XPOST "http://admin:eliquela@bbb:3000/api/annotations" \
--header "Accept: application/json" \
--header "Content-Type: application/json" -d "$(gen_body)"
