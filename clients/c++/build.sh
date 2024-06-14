#!/bin/sh

g++ -std=c++17 -DCPPHTTPLIB_OPENSSL_SUPPORT http-client.cpp -pthread -I../../lib/ -lssl -lcrypto -o http-client
