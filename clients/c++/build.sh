#!/bin/sh

g++ -std=c++17 http-client.cpp -pthread -I../../lib/ -I/usr/include/eigen3/ -lssl -lcrypto -o http-client