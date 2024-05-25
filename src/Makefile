CC=gcc

LIBXML2_INCLUDE=/usr/include/libxml2
LIBSQLITE3_INCLUDE=/usr/local/include

LIBXML2_LIB=/usr/lib
LIBSQLITE3_LIB=/usr/local/lib

OBJ_PATH=../bin/obj
BIN_PATH=../bin

${OBJ_PATH}/list.o : list.c list.h
	${CC} -c -o ${OBJ_PATH}/list.o list.c

${OBJ_PATH}/xml.o : xml.h list.h xml.c
	${CC} -I${LIBXML2_INCLUDE} -c -o ${OBJ_PATH}/xml.o xml.c

${OBJ_PATH}/sql.o : sql.h list.h sql.c
	${CC} -I${LIBSQLITE3_INCLUDE} -c -o ${OBJ_PATH}/sql.o sql.c

${OBJ_PATH}/queue.o : queue.h queue.c
	${CC} -c -o ${OBJ_PATH}/queue.o queue.c

${OBJ_PATH}/http.o : http.h http.c
	${CC} -c -o ${OBJ_PATH}/http.o http.c
 
${OBJ_PATH}/dbmgr.o : dbmgr.c sql.h xml.h defines.h queue.h
	${CC} -c -o ${OBJ_PATH}/dbmgr.o dbmgr.c

${OBJ_PATH}/datacap.o : datacap.c xml.h queue.h defines.h
	${CC} -c -o ${OBJ_PATH}/datacap.o datacap.c

${OBJ_PATH}/datasnd.o : datasnd.c xml.h queue.h defines.h http.h
	${CC} -c -o ${OBJ_PATH}/datasnd.o datasnd.c

${OBJ_PATH}/sim.o : sim.c
	${CC} -c -o ${OBJ_PATH}/sim.o sim.c

${BIN_PATH}/EMDCdbmgr : ${OBJ_PATH}/dbmgr.o ${OBJ_PATH}/sql.o ${OBJ_PATH}/xml.o ${OBJ_PATH}/list.o ${OBJ_PATH}/queue.o
	${CC} -o ${BIN_PATH}/EMDCdbmgr ${OBJ_PATH}/dbmgr.o ${OBJ_PATH}/queue.o ${OBJ_PATH}/sql.o \
                ${OBJ_PATH}/xml.o ${OBJ_PATH}/list.o -L${LIBSQLITE3_LIB} \
		-lsqlite3 -lxml2 -lzlog -liniparser -lpthread -lrt

${BIN_PATH}/EMDCdatacap : ${OBJ_PATH}/datacap.o ${OBJ_PATH}/xml.o ${OBJ_PATH}/list.o ${OBJ_PATH}/queue.o
	${CC} -o ${BIN_PATH}/EMDCdatacap ${OBJ_PATH}/datacap.o ${OBJ_PATH}/queue.o \
        ${OBJ_PATH}/xml.o ${OBJ_PATH}/list.o -lwiringPi -lxml2 -lzlog -liniparser -lpthread -lrt

${BIN_PATH}/EMDCdatasnd : ${OBJ_PATH}/datasnd.o ${OBJ_PATH}/xml.o ${OBJ_PATH}/list.o ${OBJ_PATH}/queue.o ${OBJ_PATH}/http.o
	${CC} -o ${BIN_PATH}/EMDCdatasnd ${OBJ_PATH}/datasnd.o ${OBJ_PATH}/queue.o \
        ${OBJ_PATH}/xml.o ${OBJ_PATH}/list.o ${OBJ_PATH}/http.o -lxml2 -lzlog -liniparser -lpthread -lrt -lcurl

${BIN_PATH}/EMDCsim : ${OBJ_PATH}/sim.o
	${CC} -o ${BIN_PATH}/EMDCsim ${OBJ_PATH}/sim.o -lwiringPi

${OBJ_PATH}/test_list.o : tests/test_list.c list.h
	${CC} -c -o ${OBJ_PATH}/test_list.o tests/test_list.c

${BIN_PATH}/tests/test_list : ${OBJ_PATH}/test_list.o ${OBJ_PATH}/list.o
	${CC} -o ${BIN_PATH}/tests/test_list ${OBJ_PATH}/test_list.o ${OBJ_PATH}/list.o

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

