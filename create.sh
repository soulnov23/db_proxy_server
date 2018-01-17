#!/bin/sh
SRC_DIR=./protobuf
DST_DIR=./protobuf

#C++
protoc -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/*.proto
