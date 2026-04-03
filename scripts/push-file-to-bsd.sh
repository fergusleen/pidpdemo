#!/bin/sh

set -e

SCRIPT_DIR=`cd "\`dirname "$0"\`" && pwd`
if [ -f "$SCRIPT_DIR/local-config.sh" ]; then
    . "$SCRIPT_DIR/local-config.sh"
fi

if [ $# -ne 2 ]; then
    echo "usage: $0 local_file remote_path" >&2
    exit 1
fi

LOCAL_FILE=$1
REMOTE_PATH=$2

if [ ! -f "$LOCAL_FILE" ]; then
    echo "push-file-to-bsd.sh: local file not found: $LOCAL_FILE" >&2
    exit 1
fi

case "$LOCAL_FILE" in
    *" "*|*"'"*|*\"*|*'`'*)
        echo "push-file-to-bsd.sh: local file path is too complex: $LOCAL_FILE" >&2
        exit 1
        ;;
esac

case "$REMOTE_PATH" in
    /*|*..*|*" "*|*"'"*|*\"*|*'`'*|*'|'*|*';'*|*'&'*|*'<'*|*'>'*|*'$'*|*'('*|*')'*)
        echo "push-file-to-bsd.sh: unsafe remote path: $REMOTE_PATH" >&2
        exit 1
        ;;
esac

TARGET_HOST=${TARGET_HOST:-example-host}
TARGET_PORT=${TARGET_PORT:-2323}
TARGET_USER=${TARGET_USER:-user}
TARGET_DIR=${TARGET_DIR:-pidpdemo}
TARGET_PROJECT=${TARGET_PROJECT:-pidpdemo}
TARGET_PROJECT_DIR=${TARGET_PROJECT_DIR:-$TARGET_DIR/$TARGET_PROJECT}
TARGET_PROMPT=${TARGET_PROMPT:-'[\$%#>] $'}
TARGET_SH_PROMPT=${TARGET_SH_PROMPT:-'[\$#] $'}
TELNET_CMD=${TELNET_CMD:-telnet}
CHUNK_LINES=${CHUNK_LINES:-80}

BASE_DIR=`cd "$SCRIPT_DIR/.." && pwd`
LOG_DIR="$BASE_DIR/logs"
CHUNK_DIR=

mkdir -p "$LOG_DIR"
umask 077
CHUNK_DIR="$LOG_DIR/push-chunks.$$"
mkdir -p "$CHUNK_DIR"
trap 'rm -rf "$CHUNK_DIR"' 0 1 2 3 15

awk -v lines="$CHUNK_LINES" -v prefix="$CHUNK_DIR/chunk." '
{
    file = sprintf("%s%04d", prefix, int((NR - 1) / lines));
    print >> file;
}
' "$LOCAL_FILE"

export TARGET_HOST TARGET_PORT TARGET_USER TARGET_PROJECT_DIR
export TARGET_PROMPT TARGET_SH_PROMPT TELNET_CMD REMOTE_PATH CHUNK_DIR

expect <<'EOF'
set timeout 300
log_user 0

set host $env(TARGET_HOST)
set port $env(TARGET_PORT)
set user $env(TARGET_USER)
set project_dir $env(TARGET_PROJECT_DIR)
set prompt $env(TARGET_PROMPT)
set sh_prompt $env(TARGET_SH_PROMPT)
set telnet_cmd $env(TELNET_CMD)
set remote_path $env(REMOTE_PATH)
set chunk_dir $env(CHUNK_DIR)

proc slurp {path} {
    set fh [open $path r]
    set data [read $fh]
    close $fh
    return $data
}

set chunks [lsort [glob -nocomplain [file join $chunk_dir chunk.*]]]

spawn $telnet_cmd $host $port
expect {
    -re "(?i)login: $" {}
    timeout { send_user "No login prompt seen.\n"; exit 1 }
    eof { send_user "Connection closed before login.\n"; exit 1 }
}
send -- "$user\r"
expect {
    -re $prompt {}
    timeout { send_user "Shell prompt not seen after login.\n"; exit 1 }
    eof { send_user "Connection closed after login.\n"; exit 1 }
}

send -- "sh\r"
expect {
    -re $sh_prompt {}
    timeout { send_user "Plain sh prompt not seen after starting sh.\n"; exit 1 }
    eof { send_user "Connection closed before entering sh.\n"; exit 1 }
}

send -- "cd $project_dir || exit 1\r"
expect {
    -re $sh_prompt {}
    timeout { send_user "Could not change directory to $project_dir.\n"; exit 1 }
    eof { send_user "Connection closed before file push.\n"; exit 1 }
}

send -- ": > $remote_path\r"
expect {
    -re $sh_prompt {}
    timeout { send_user "Could not create $remote_path on the target.\n"; exit 1 }
    eof { send_user "Connection closed before chunk upload.\n"; exit 1 }
}

set index 0
foreach chunk $chunks {
    set tag [format "__PIDP_CHUNK_%04d__" $index]
    set payload [slurp $chunk]
    send -- "cat >> $remote_path <<'$tag'\r"
    send -- $payload
    if {[string length $payload] == 0 || [string index $payload end] ne "\n"} {
        send -- "\r"
    }
    send -- "$tag\r"
    expect {
        -re $sh_prompt {}
        timeout {
            send_user "Chunk upload did not return to the shell prompt.\n"
            exit 1
        }
        eof {
            send_user "Connection closed during chunk upload.\n"
            exit 1
        }
    }
    incr index
}

send -- "echo __PIDP_PUSH_DONE__\r"
expect {
    "__PIDP_PUSH_DONE__" {}
    timeout { send_user "Upload finished but completion marker was not seen.\n"; exit 1 }
    eof { send_user "Connection closed before upload completion was confirmed.\n"; exit 1 }
}

send -- "\035"
expect {
    "telnet>" {}
    timeout { send_user "Could not enter telnet command mode to close the session.\n"; exit 1 }
    eof { exit 0 }
}
send -- "quit\r"
expect eof
send_user "Upload complete.\n"
EOF

echo "Pushed $LOCAL_FILE to $TARGET_PROJECT_DIR/$REMOTE_PATH"
