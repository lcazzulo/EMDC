#!/bin/bash

export DC_USER=emdc
export DC_GROUP=emdc
export EMDC_HOME=/home/emdc
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib:${EMDC_HOME}/lib
while [ 1 ]
do
	nohup ${EMDC_HOME}/bin/EMDCdatacap --dc-id=$1
done
exit 0
