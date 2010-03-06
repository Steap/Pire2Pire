#!/bin/bash

# Absolute link to the bin directory
DIR=$(cd $(dirname $0); pwd -P)

# Absolute link to this script
SELF=$(cd $(dirname $0); pwd -P)/$(basename $0)

# Absolute link to the daemon
P2P_DAEMON=${DIR}/daemon

# Absolute link to the daemon conf file.
# TODO : irl, it would be /etc/<app>/daemon.conf
DAEMON_CONF_FILE=${DIR}/../conf/daemon.conf

LOCK_FILE=/tmp/k
function usage {
    echo "Usage :"
    echo -n "$(cd $(dirname $0); pwd -P)/$(basename $0) "
    echo "[start | stop | restart | status]"
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
#    touch $LOCK_FILE
    # The absolute link of the daemon's conf file is passed as an argument. It
    # does suck ass, but eh... Marked as FIXME !
    $P2P_DAEMON $DAEMON_CONF_FILE
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
        # The daemon might not have deleted the lock file itself (in case it
        # receives SIGKILL before unlinking. We shall then delete it ourselves.
        if [ -r $LOCK_FILE ]
        then
            rm $LOCK_FILE
        fi
    fi
}

function server_restart {
    echo ' * About to restart the daemon...'
    $SELF stop && $SELF start; 
}

function server_status {
    if [ ! -f $LOCK_FILE ]
    then
        echo "The daemon is down.";
    else
        echo "The daemon is running with PID `cat ${LOCK_FILE}`"
    fi
}
# Main
#echo "DIR is ${DIR}"
#echo "DAEMON is ${P2P_DAEMON}"
#exit 0
case "${1:-''}" in
    'restart')
        server_restart;
        ;;
    'start')
        server_start;
        ;;
    'status')
        server_status;
        ;;
    'stop')
        server_stop; 
        ;;
    *)
        usage;
esac


