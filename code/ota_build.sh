#!/bin/bash
#
# Older OTA path: build, serve firmware.bin from a throwaway nginx container, and
# publish the MQTT trigger. ota_push.sh (scp to Home Assistant) is the maintained
# route; this is kept for the no-HA case.
#
# Usage: ./ota_build.sh <128x64|64x64>

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

variant=${1:-}
case $variant in
  128x64|64x64) ;;
  *)
    echo "Usage: $0 <128x64|64x64>" >&2
    exit 2
    ;;
esac
variant_suffix=${variant^^}

# shellcheck disable=SC1091
. creds_ota.sh

# MQTT_TOPIC is per-variant (see creds_ota.sh.sample); the unsuffixed name is a
# fallback for 128x64 only, so that pre-split creds files keep working.
topic_var="MQTT_TOPIC_${variant_suffix}"
mqtt_topic=${!topic_var:-}
if [[ -z $mqtt_topic && $variant == "128x64" ]]; then
  mqtt_topic=${MQTT_TOPIC:-}
fi
if [[ -z $mqtt_topic ]]; then
  echo "ERROR: set $topic_var in creds_ota.sh." >&2
  exit 1
fi

pio run -e "$variant"

firmware_bin=".pio/build/${variant}/firmware.bin"
if [[ ! -f $firmware_bin ]]; then
  echo "ERROR: Build succeeded but $firmware_bin was not created." >&2
  exit 1
fi

cp "$firmware_bin" "/tmp/$MQTT_ESP_HOSTNAME.bin"

echo "Starting container..."

CONTAINER_ID=$(docker container run -d -p 8080:80 --rm -v "/tmp/$MQTT_ESP_HOSTNAME.bin:/usr/share/nginx/html/$MQTT_ESP_HOSTNAME.bin" nginx:latest)
trap 'echo "Stopping container!"; docker stop "$CONTAINER_ID" >/dev/null' EXIT

echo "Sending MQTT OTA request for $variant"

mosquitto_pub -h "$MQTT_HOST" -u "$MQTT_USER" -P "$MQTT_PASS" -t "$mqtt_topic" -m "$MQTT_PAYLOAD"

echo "Waiting for OTA"
sleep "$WAIT_TIME_SEC"

echo "Done!"
