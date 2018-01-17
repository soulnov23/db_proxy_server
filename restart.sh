#!/bin/bash

while true
do 
    procnum=" `pidof "db_proxy_server"|wc -l` "
    if [ $procnum -eq 0 ]; then
       	killall -9 db_proxy_server
	 ./db_proxy_server&
    fi
    sleep 5
done

