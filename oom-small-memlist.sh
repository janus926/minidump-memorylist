#!/bin/bash

api_token=

reports="$(curl -s 'https://crash-stats.mozilla.com/api/SuperSearch/?release_channel=Nightly&signature=%22OOM%20%7C%20small%22&product=Firefox' | python -c "exec('import json,sys;obj=json.load(sys.stdin);\\nfor hit in obj[\"hits\"]: print hit[\"uuid\"]')")"

for r in ${reports}; do
  curl -s -H 'Auth-Token:'$api_token 'https://crash-stats.mozilla.com/api/RawCrash/?format=raw&crash_id='$r -o $r.dmp
  if [ -f $r.dmp ]; then
    analysis="$(./minidump-memorylist ${r}.dmp)"
    rm $r.dmp
  fi
  echo $r,${analysis:-n/a}
done
