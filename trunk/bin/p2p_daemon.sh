#!/bin/bash

DIR=$(cd $(dirname $0); pwd -P)
SELF=$(cd $(dirname $0); pwd -P)/$(basename $0)
P2P_DAEMON=${DIR}/daemon

LOCK_FILE=/tmp/k
function usage {
    echo "Usage :"
    echo -n "$(cd $(dirname $0); pwd -P)/$(basename $0) "
    echo "[start | stop | restart]"
}

function server_start {
    if [ ! -x $P2P_DAEMON ]
    then
        echo 'The daemon does not exist !';
        exit 1;
    fi
    if [ -f $LOCK_FILE ]
    then
        echo '[FAIL] It seems like the daemon is already running';
        exit 1;
    fi
    touch $LOCK_FILE
    $P2P_DAEMON
    echo ' * Daemon started'
}

function server_stop {
    if [ ! -r $LOCK_FILE ]
    then
        echo 'The server does not seem to be launched...';
    else
        pid=`cat ${LOCK_FILE}`;
        echo " * Sending SIGTERM to process ${pid}";
        kill -TERM `cat ${LOCK_FILE}`;
        echo " * Sending SIGKILL to process ${pid}";
        kill -KILL $pid;
        rm -f $LOCK_FILE
    fi
}

function server_restart {
    echo ' * About to restart the daemon...'
    $SELF stop && $SELF start; 
}

# Main
case "${1:-''}" in
    'restart')
        server_restart;
        ;;
    'start')
        server_start;
        ;;
    'stop')
        server_stop; 
        ;;
    *)
        usage;
esac


