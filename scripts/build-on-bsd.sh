#!/bin/sh

set -e

SCRIPT_DIR=`cd "\`dirname "$0"\`" && pwd`
if [ -f "$SCRIPT_DIR/local-config.sh" ]; then
    . "$SCRIPT_DIR/local-config.sh"
fi

TARGET_HOST=${TARGET_HOST:-example-host}
TARGET_PORT=${TARGET_PORT:-2323}
TARGET_USER=${TARGET_USER:-user}
TARGET_DIR=${TARGET_DIR:-pidpdemo}
TARGET_PROJECT=${TARGET_PROJECT:-pidpdemo}
TARGET_PROJECT_DIR=${TARGET_PROJECT_DIR:-$TARGET_DIR/$TARGET_PROJECT}
TARGET_PROMPT=${TARGET_PROMPT:-'[\$%#>] $'}
TARGET_SH_PROMPT=${TARGET_SH_PROMPT:-'[\$#] $'}
TELNET_CMD=${TELNET_CMD:-telnet}

USE_CURSES=${USE_CURSES:-0}
BUILD_CMD=${BUILD_CMD:-make clean all}
if [ "$USE_CURSES" = "1" ]; then
    BUILD_CMD=${BUILD_CMD_OVERRIDE:-make clean all CPPFLAGS='-DUSE_CURSES=1' LDLIBS='-lcurses -ltermcap'}
fi

BASE_DIR=`cd "$SCRIPT_DIR/.." && pwd`
LOG_DIR="$BASE_DIR/logs"
STAMP=`date +%Y%m%d-%H%M%S`
LOG_FILE="$LOG_DIR/build-$STAMP.log"

mkdir -p "$LOG_DIR"

export TARGET_HOST TARGET_PORT TARGET_USER TARGET_PROJECT_DIR
export TARGET_PROMPT TARGET_SH_PROMPT TELNET_CMD BUILD_CMD LOG_FILE

expect -f "$SCRIPT_DIR/telnet-build.exp"

echo "Build log saved to $LOG_FILE"
