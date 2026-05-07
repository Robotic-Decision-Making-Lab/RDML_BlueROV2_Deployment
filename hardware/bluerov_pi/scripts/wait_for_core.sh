#!/bin/bash

wait_for_core() {
    local container_name=$1
    local retries=$2
    local sleep_time=$3
    local post_start_sleep=30

    for ((i=0; i<retries; i++)); do
        if [ "$( docker container inspect -f '{{.State.Status}}' $container_name )" = "running" ]; then
            echo "[INFO] $container_name is running"
            sleep $post_start_sleep
            return 0
        fi

        echo "[INFO] Waiting for $container_name to start... ($((i+1))/$retries)"
        sleep $sleep_time
    done

    echo "[ERROR] $container_name did not start within the expected time."
    return 1
}

# the blueos-core container starts ~60 seconds after the system boots
wait_for_core "blueos-core" 20 5
