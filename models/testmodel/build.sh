#!/bin/sh

g++ minimal-server.cpp -pthread -lssl -lcrypto -o minimal-server
