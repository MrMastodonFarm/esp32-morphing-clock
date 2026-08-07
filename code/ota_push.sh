#!/bin/bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

readonly VALID_VARIANTS=(128x64 64x64)

usage() {
  cat >&2 <<'EOF'
Usage: ota_push.sh <variant> [--dry-run]

  variant     128x64  the wide clock
              64x64   the square clock

The variant is required rather than defaulted: pushing one clock's binary to the
other is a bricking-class mistake, so the target is always named explicitly.

Per-variant settings are read from ./creds_ota.sh. Each of HA_WWW_DIR and MQTT_TOPIC
may be suffixed (HA_WWW_DIR_64X64, MQTT_TOPIC_128X64, ...); for 128x64 the unsuffixed
name is accepted as a fallback so existing creds files keep working.
EOF
  exit 2
}

variant=""
dry_run=false
for arg in "$@"; do
  case $arg in
    --dry-run) dry_run=true ;;
    -h|--help) usage ;;
    -*) echo "ERROR: unknown option '$arg'." >&2; usage ;;
    *)
      if [[ -n $variant ]]; then
        echo "ERROR: more than one variant given ('$variant' and '$arg')." >&2
        usage
      fi
      variant=$arg
      ;;
  esac
done

if [[ -z $variant ]]; then
  echo "ERROR: no variant given." >&2
  usage
fi

variant_ok=false
for v in "${VALID_VARIANTS[@]}"; do
  [[ $variant == "$v" ]] && variant_ok=true
done
if [[ $variant_ok == false ]]; then
  echo "ERROR: unknown variant '$variant'. Valid: ${VALID_VARIANTS[*]}" >&2
  usage
fi

# 128x64 -> 128X64, used to look up the per-variant creds variables
variant_suffix=${variant^^}

if [[ ! -f ./creds_ota.sh ]]; then
  echo "ERROR: ./creds_ota.sh is missing (copy creds_ota.sh.sample and fill it in)." >&2
  exit 1
fi

# shellcheck disable=SC1091
source ./creds_ota.sh

# Resolve a setting that may be per-variant. Falls back to the unsuffixed name only
# for 128x64, whose values predate the split.
resolve_setting() {
  local base=$1 suffixed="${1}_${variant_suffix}"
  if [[ -n ${!suffixed+x} && -n ${!suffixed} ]]; then
    printf '%s' "${!suffixed}"
    return 0
  fi
  if [[ $variant == "128x64" && -n ${!base+x} && -n ${!base} ]]; then
    printf '%s' "${!base}"
    return 0
  fi
  return 1
}

readonly SHARED_VARS=(HA_SSH_HOST HA_SSH_PORT HA_SSH_USER MQTT_HOST MQTT_USER MQTT_PASS MQTT_PAYLOAD)

missing_vars=()
for var_name in "${SHARED_VARS[@]}"; do
  if [[ -z ${!var_name+x} || -z ${!var_name} ]]; then
    missing_vars+=("$var_name")
  fi
done

ha_www_dir=""
mqtt_topic=""
ha_www_dir=$(resolve_setting HA_WWW_DIR) || missing_vars+=("HA_WWW_DIR_${variant_suffix}")
mqtt_topic=$(resolve_setting MQTT_TOPIC) || missing_vars+=("MQTT_TOPIC_${variant_suffix}")

if (( ${#missing_vars[@]} > 0 )); then
  echo "ERROR: ./creds_ota.sh has unset or empty required variables: ${missing_vars[*]}" >&2
  exit 1
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

echo "Target: $variant"
pio run -e "$variant"

firmware_bin=".pio/build/${variant}/firmware.bin"
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

scp_destination="${HA_SSH_USER}@${HA_SSH_HOST}:${ha_www_dir}/firmware.bin"

if [[ $dry_run == true ]]; then
  printf 'DRY RUN: '
  printf '%q ' scp -P "$HA_SSH_PORT" "$firmware_bin" "$scp_destination"
  printf '\n'
  printf 'DRY RUN: '
  printf '%q ' mosquitto_pub -h "$MQTT_HOST" -u "$MQTT_USER" -P '********' -t "$mqtt_topic" -m "$MQTT_PAYLOAD"
  printf '\n'
  echo "Dry run complete; OTA was not triggered."
  exit 0
fi

scp -P "$HA_SSH_PORT" "$firmware_bin" "$scp_destination"
mosquitto_pub -h "$MQTT_HOST" -u "$MQTT_USER" -P "$MQTT_PASS" -t "$mqtt_topic" -m "$MQTT_PAYLOAD"

echo "OTA triggered for $variant. The clock should reboot within ~30s; a few minutes of MQTT reconnect churn after reboot is normal."
