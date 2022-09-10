#!/bin/bash

#DATE="22-Sep-2014 10:32:35.012"
DATE="$1"
timestamp=$(date -d "$DATE" +'%s000')

function gen_body(){
  cat <<EOF
{
  "dashboardUID":"LlUpHMggk",
  "panelId":2,
  "time":$timestamp,
  "text":"Annotation Description"
}
EOF
}

echo $(gen_body)

curl -XPOST "http://admin:eliquela@bbb:3000/api/annotations" \
--header "Accept: application/json" \
--header "Content-Type: application/json" -d "$(gen_body)"
