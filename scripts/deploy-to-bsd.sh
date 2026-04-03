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
TARGET_PROMPT=${TARGET_PROMPT:-'[\$%#>] $'}
TARGET_SH_PROMPT=${TARGET_SH_PROMPT:-'[\$#] $'}
TELNET_CMD=${TELNET_CMD:-telnet}

BASE_DIR=`cd "$SCRIPT_DIR/.." && pwd`
LOG_DIR="$BASE_DIR/logs"
UPLOAD_SCRIPT="$LOG_DIR/upload-remote.sh"
FILES="README.md Makefile \
src/main.c src/menu.c src/menu.h src/pager.c src/pager.h src/actions.c src/actions.h src/compat.h \
pages/intro.txt pages/pdp11.txt pages/unix.txt pages/bsd211.txt pages/panel.txt pages/games.txt \
scripts/deploy-to-bsd.sh scripts/build-on-bsd.sh scripts/run-on-bsd.sh scripts/smoke-test.sh \
scripts/telnet-build.exp scripts/telnet-run.exp scripts/push-file-to-bsd.sh scripts/sysinfo.sh"

mkdir -p "$LOG_DIR"

cd "$BASE_DIR"

(
    echo "#!/bin/sh"
    echo "umask 022"
    echo "test -d \"$TARGET_DIR\" || mkdir \"$TARGET_DIR\""
    echo "cd \"$TARGET_DIR\" || exit 1"
    echo "test -d \"$TARGET_PROJECT\" || mkdir \"$TARGET_PROJECT\""
    echo "cd \"$TARGET_PROJECT\" || exit 1"
    echo "test -d src || mkdir src"
    echo "test -d pages || mkdir pages"
    echo "test -d scripts || mkdir scripts"
    echo "test -d logs || mkdir logs"
    for file in $FILES; do
        echo "cat > \"$file\" <<'__PIDPDEMO_EOF__'"
        cat "$file"
        echo "__PIDPDEMO_EOF__"
    done
    echo "echo __PIDPDEMO_UPLOAD_DONE__"
) > "$UPLOAD_SCRIPT"

export TARGET_HOST TARGET_PORT TARGET_USER TARGET_PROMPT
export TARGET_SH_PROMPT TELNET_CMD UPLOAD_SCRIPT

expect <<'EOF'
set timeout 300

set host $env(TARGET_HOST)
set port $env(TARGET_PORT)
set user $env(TARGET_USER)
set prompt $env(TARGET_PROMPT)
set sh_prompt $env(TARGET_SH_PROMPT)
set telnet_cmd $env(TELNET_CMD)
set upload_script $env(UPLOAD_SCRIPT)

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

set fh [open $upload_script r]
set payload [read $fh]
close $fh

send -- "$payload\r"
expect {
    "__PIDPDEMO_UPLOAD_DONE__" {}
    timeout { send_user "Upload did not complete.\n"; exit 1 }
    eof { send_user "Connection closed during upload.\n"; exit 1 }
}
send -- "exit\r"
expect {
    -re $prompt {}
    timeout { send_user "Did not return to the login shell after leaving sh.\n"; exit 1 }
    eof { send_user "Connection closed before returning to the login shell.\n"; exit 1 }
}
send -- "exit\r"
expect eof
EOF

echo "Remote upload completed to $TARGET_DIR/$TARGET_PROJECT"
