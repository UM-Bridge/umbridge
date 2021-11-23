#!/bin/sh

g++ minimal-server.cpp -pthread -I/usr/include/eigen3/ -lssl -lcrypto -o minimal-server
