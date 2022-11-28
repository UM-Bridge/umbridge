#!/bin/sh

g++ -std=c++17 http-client.cpp -pthread -I../../lib/ -lssl -lcrypto -o http-client
