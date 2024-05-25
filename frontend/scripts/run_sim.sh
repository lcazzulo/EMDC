#!/bin/bash

export LD_LIBRARY_PATH=/usr/local/lib
export EMDC_HOME=/home/emdc
nohup ${EMDC_HOME}/bin/EMDCsim &
exit 0
