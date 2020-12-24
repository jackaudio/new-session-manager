#!/bin/bash

if [[ -z "$1" ]]; then
    echo "Give NSM OSC Port as only parameter. Afterwards you can run the executable pythons in this dir, directly, without ./"
    exit 1
fi

export NSM_URL=osc.udp://0.0.0.0:$1/
export PATH=$(pwd):$PATH
