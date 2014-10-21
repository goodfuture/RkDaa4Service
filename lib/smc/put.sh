###############################################################################
#: File Name        : put.sh
#: Author           : KangQi
#: E-Mail           : good_future@sina.cn
#: Created Time     : 2014-10-21 12:50
#: Last Modified    : 2014-10-21 12:50
###############################################################################
#!/bin/bash -e

HOST_ADDR=192.168.205.251
HOST_PATH=/usr/lib/extend
scp libsmc.so root@$HOST_ADDR:$HOST_PATH
