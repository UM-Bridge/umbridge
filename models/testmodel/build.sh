#!/bin/sh

g++ minimal-server.cpp -pthread -I../../lib/ -I/usr/include/eigen3/ -lssl -lcrypto -o minimal-server
