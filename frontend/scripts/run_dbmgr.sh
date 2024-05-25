#!/bin/bash

export EMDC_HOME=/home/emdc
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib:${EMDC_HOME}/lib
cd ${EMDC_HOME}/log
while [ 1 ]
do
	nohup ${EMDC_HOME}/bin/EMDCdbmgr
done
exit 0
