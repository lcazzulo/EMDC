CC=gcc

LIB_INIPARSER_INCLUDE=/usr/local/include/iniparser
LIB_JSON_C_INCLUDE=/usr/local/include/json-c
LIB_RABBIT_MQ_C_INCLUDE=../3rdParty/rabbitmq-c/include
LIBXML2_INCLUDE=/usr/include/libxml2
LIBSQLITE3_INCLUDE=/usr/local/include

LIBXML2_LIB=/usr/lib
LIBSQLITE3_LIB=/usr/local/lib
LIB_RABBIT_MQ_C_LIB=../3rdParty/rabbitmq-c/librabbitmq

OBJ_PATH=../bin/obj
BIN_PATH=../bin

${OBJ_PATH}/list.o : list.c list.h
	${CC} -c -o ${OBJ_PATH}/list.o list.c

${OBJ_PATH}/my_json.o : my_json.c my_json.h
	${CC} -I${LIB_JSON_C_INCLUDE} -c -o ${OBJ_PATH}/my_json.o my_json.c

${OBJ_PATH}/my_amqp.o : my_amqp.c my_amqp.h
	${CC} -I${LIB_RABBIT_MQ_C_INCLUDE} -c -o ${OBJ_PATH}/my_amqp.o my_amqp.c

${OBJ_PATH}/sql.o : sql.h list.h sql.c
	${CC} -I${LIBSQLITE3_INCLUDE} -c -o ${OBJ_PATH}/sql.o sql.c

${OBJ_PATH}/queue.o : queue.h queue.c
	${CC} -c -o ${OBJ_PATH}/queue.o queue.c

${OBJ_PATH}/http.o : http.h http.c
	${CC} -c -o ${OBJ_PATH}/http.o http.c

${OBJ_PATH}/dbmgr.o : dbmgr.c sql.h defines.h queue.h
	${CC} -I${LIB_INIPARSER_INCLUDE} -c -o ${OBJ_PATH}/dbmgr.o dbmgr.c

${OBJ_PATH}/datacap.o : datacap.c xml.h queue.h defines.h
	${CC} -I${LIB_INIPARSER_INCLUDE} -c -o ${OBJ_PATH}/datacap.o datacap.c

${OBJ_PATH}/datapublish.o : datapublish.c queue.h defines.h
	${CC} -I${LIB_INIPARSER_INCLUDE} -I${LIB_RABBIT_MQ_C_INCLUDE} -c -o ${OBJ_PATH}/datapublish.o datapublish.c

${OBJ_PATH}/datasnd.o : datasnd.c xml.h queue.h defines.h http.h
	${CC} -c -o ${OBJ_PATH}/datasnd.o datasnd.c

${OBJ_PATH}/sim.o : sim.c
	${CC} -c -o ${OBJ_PATH}/sim.o sim.c

${BIN_PATH}/EMDCdbmgr : ${OBJ_PATH}/dbmgr.o ${OBJ_PATH}/sql.o ${OBJ_PATH}/queue.o ${OBJ_PATH}/my_json.o
	${CC} -o ${BIN_PATH}/EMDCdbmgr ${OBJ_PATH}/dbmgr.o ${OBJ_PATH}/queue.o ${OBJ_PATH}/sql.o ${OBJ_PATH}/my_json.o \
                 -L${LIBSQLITE3_LIB} -lsqlite3 -lzlog -liniparser -ljson-c -lpthread -lrt

${BIN_PATH}/EMDCdatacap : ${OBJ_PATH}/datacap.o ${OBJ_PATH}/queue.o ${OBJ_PATH}/my_json.o
	${CC} -o ${BIN_PATH}/EMDCdatacap ${OBJ_PATH}/datacap.o ${OBJ_PATH}/queue.o \
        ${OBJ_PATH}/my_json.o -lwiringPi -lzlog -liniparser -ljson-c -lpthread -lrt

${BIN_PATH}/EMDCdatapublish : ${OBJ_PATH}/datapublish.o ${OBJ_PATH}/queue.o ${OBJ_PATH}/my_json.o ${OBJ_PATH}/my_amqp.o
	${CC} -o ${BIN_PATH}/EMDCdatapublish ${OBJ_PATH}/datapublish.o ${OBJ_PATH}/queue.o \
	${OBJ_PATH}/my_json.o ${OBJ_PATH}/my_amqp.o -lzlog -liniparser -ljson-c -L${LIB_RABBIT_MQ_C_LIB} -lrabbitmq -lpthread -lrt

${BIN_PATH}/EMDCdatasnd : ${OBJ_PATH}/datasnd.o ${OBJ_PATH}/xml.o ${OBJ_PATH}/list.o ${OBJ_PATH}/queue.o ${OBJ_PATH}/http.o
	${CC} -o ${BIN_PATH}/EMDCdatasnd ${OBJ_PATH}/datasnd.o ${OBJ_PATH}/queue.o \
        ${OBJ_PATH}/xml.o ${OBJ_PATH}/list.o ${OBJ_PATH}/http.o -lxml2 -lzlog -liniparser -lpthread -lrt -lcurl

${BIN_PATH}/EMDCsim : ${OBJ_PATH}/sim.o
	${CC} -o ${BIN_PATH}/EMDCsim ${OBJ_PATH}/sim.o -lwiringPi

${OBJ_PATH}/test_list.o : tests/test_list.c list.h
	${CC} -c -o ${OBJ_PATH}/test_list.o tests/test_list.c

${BIN_PATH}/tests/test_list : ${OBJ_PATH}/test_list.o ${OBJ_PATH}/list.o
	${CC} -o ${BIN_PATH}/tests/test_list ${OBJ_PATH}/test_list.o ${OBJ_PATH}/list.o

${OBJ_PATH}/test_json.o : tests/test_json.c my_json.h
	${CC} -I${LIB_JSON_C_INCLUDE} -c -o ${OBJ_PATH}/test_json.o tests/test_json.c

${OBJ_PATH}/test_my_amqp.o : tests/test_my_amqp.c my_amqp.h
	${CC} -I${LIB_RABBIT_MQ_C_INCLUDE} -c -o ${OBJ_PATH}/test_my_amqp.o tests/test_my_amqp.c

${BIN_PATH}/tests/test_json : ${OBJ_PATH}/test_json.o ${OBJ_PATH}/my_json.o
	${CC} -o ${BIN_PATH}/tests/test_json ${OBJ_PATH}/test_json.o ${OBJ_PATH}/my_json.o -ljson-c -lzlog -lpthread

${BIN_PATH}/tests/test_my_amqp : ${OBJ_PATH}/test_my_amqp.o ${OBJ_PATH}/my_amqp.o
	${CC} -o ${BIN_PATH}/tests/test_my_amqp ${OBJ_PATH}/test_my_amqp.o ${OBJ_PATH}/my_amqp.o -L${LIB_RABBIT_MQ_C_LIB} -lrabbitmq -lzlog -lpthread

${OBJ_PATH}/test_xml.o : tests/test_xml.c xml.h list.h
	${CC} -c -o ${OBJ_PATH}/test_xml.o tests/test_xml.c

${BIN_PATH}/tests/test_xml : ${OBJ_PATH}/test_xml.o ${OBJ_PATH}/xml.o ${OBJ_PATH}/list.o
	${CC} -o ${BIN_PATH}/tests/test_xml ${OBJ_PATH}/test_xml.o ${OBJ_PATH}/xml.o ${OBJ_PATH}/list.o -lxml2 -lzlog -lpthread

${OBJ_PATH}/test_sql.o : tests/test_sql.c sql.h list.h
	${CC} -c -o ${OBJ_PATH}/test_sql.o tests/test_sql.c

${BIN_PATH}/tests/test_sql : ${OBJ_PATH}/test_sql.o ${OBJ_PATH}/sql.o ${OBJ_PATH}/list.o
	${CC} -o ${BIN_PATH}/tests/test_sql ${OBJ_PATH}/test_sql.o ${OBJ_PATH}/sql.o \
        ${OBJ_PATH}/xml.o ${OBJ_PATH}/list.o -L${LIBSQLITE3_LIB} -lsqlite3 -lxml2 -lzlog -lpthread

${OBJ_PATH}/test_http.o : tests/test_http.c http.h
	${CC} -c -o ${OBJ_PATH}/test_http.o tests/test_http.c

${BIN_PATH}/tests/test_http : ${OBJ_PATH}/test_http.o ${OBJ_PATH}/http.o
	${CC} -o ${BIN_PATH}/tests/test_http ${OBJ_PATH}/test_http.o ${OBJ_PATH}/http.o -lcurl -lzlog -lpthread

clean:
	rm -f ${OBJ_PATH}/*.o
	rm -f ${BIN_PATH}/EMDCdbmgr
	rm -f ${BIN_PATH}/EMDCdatacap
	rm -f ${BIN_PATH}/EMDCdatasnd
	rm -f ${BIN_PATH}/EMDCsim
	rm -f ${BIN_PATH}/tests/test_list
	rm -f ${BIN_PATH}/tests/test_xml
	rm -f ${BIN_PATH}/tests/test_sql
	rm -f ${BIN_PATH}/tests/test_http
	rm -f ${BIN_PATH}/tests/test_json

