#!/bin/bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

readonly REQUIRED_VARS=(
  HA_SSH_HOST
  HA_SSH_PORT
  HA_SSH_USER
  HA_WWW_DIR
  MQTT_HOST
  MQTT_USER
  MQTT_PASS
  MQTT_TOPIC
  MQTT_PAYLOAD
)

if [[ ! -f ./creds_ota.sh ]]; then
  echo "ERROR: ./creds_ota.sh is missing." >&2
  echo "Required variables: ${REQUIRED_VARS[*]}" >&2
  exit 1
fi

# shellcheck disable=SC1091
source ./creds_ota.sh

missing_vars=()
for var_name in "${REQUIRED_VARS[@]}"; do
  if [[ -z ${!var_name+x} || -z ${!var_name} ]]; then
    missing_vars+=("$var_name")
  fi
done

if (( ${#missing_vars[@]} > 0 )); then
  echo "ERROR: ./creds_ota.sh has unset or empty required variables: ${missing_vars[*]}" >&2
  echo "Required variables: ${REQUIRED_VARS[*]}" >&2
  exit 1
fi

dry_run=false
if (( $# > 1 )); then
  echo "Usage: $0 [--dry-run]" >&2
  exit 2
fi
if (( $# == 1 )); then
  if [[ $1 != "--dry-run" ]]; then
    echo "Usage: $0 [--dry-run]" >&2
    exit 2
  fi
  dry_run=true
fi

OTA_SIZE_CAP_BYTES=${OTA_SIZE_CAP_BYTES:-1250000}
if [[ ! $OTA_SIZE_CAP_BYTES =~ ^[0-9]+$ ]] || (( OTA_SIZE_CAP_BYTES == 0 )); then
  echo "ERROR: OTA_SIZE_CAP_BYTES must be a positive integer (got '$OTA_SIZE_CAP_BYTES')." >&2
  exit 1
fi

if ! command -v pio >/dev/null 2>&1; then
  echo "ERROR: pio is not on PATH; install or activate PlatformIO before running this script." >&2
  exit 1
fi

pio run

readonly firmware_bin=.pio/build/esp32dev/firmware.bin
if [[ ! -f $firmware_bin ]]; then
  echo "ERROR: Build succeeded but $firmware_bin was not created." >&2
  exit 1
fi

firmware_size=$(wc -c < "$firmware_bin")

# The device's real partition table is whatever the last USB flash wrote (probably
# the 1.28 MB stock default), which is smaller than the min_spiffs table declared in
# platformio.ini. PlatformIO's size check can therefore pass a build that would
# brick the device during OTA.
if (( firmware_size >= OTA_SIZE_CAP_BYTES )); then
  echo "ERROR: OTA ABORTED: firmware is $firmware_size bytes; it must be smaller than $OTA_SIZE_CAP_BYTES bytes." >&2
  exit 1
fi

firmware_md5_output=$(md5sum "$firmware_bin")
firmware_md5=${firmware_md5_output%% *}
echo "Firmware size: $firmware_size bytes"
echo "Firmware MD5:  $firmware_md5"

scp_destination="${HA_SSH_USER}@${HA_SSH_HOST}:${HA_WWW_DIR}/firmware.bin"

if [[ $dry_run == true ]]; then
  printf 'DRY RUN: '
  printf '%q ' scp -P "$HA_SSH_PORT" "$firmware_bin" "$scp_destination"
  printf '\n'
  printf 'DRY RUN: '
  printf '%q ' mosquitto_pub -h "$MQTT_HOST" -u "$MQTT_USER" -P '********' -t "$MQTT_TOPIC" -m "$MQTT_PAYLOAD"
  printf '\n'
  echo "Dry run complete; OTA was not triggered."
  exit 0
fi

scp -P "$HA_SSH_PORT" "$firmware_bin" "$scp_destination"
mosquitto_pub -h "$MQTT_HOST" -u "$MQTT_USER" -P "$MQTT_PASS" -t "$MQTT_TOPIC" -m "$MQTT_PAYLOAD"

echo "OTA triggered. The clock should reboot within ~30s; a few minutes of MQTT reconnect churn after reboot is normal."
