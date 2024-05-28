all: build-load-balancer build-testmodel

build-load-balancer:
	- g++ -O3 -Wno-unused-result -std=c++17 -I../lib/ LoadBalancer.cpp -o load-balancer -pthread

build-testmodel:
	- g++ -O3 -Wno-unused-result -std=c++17 -I../lib/ ../models/testmodel/minimal-server.cpp -o testmodel -pthread
