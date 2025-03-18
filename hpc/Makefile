all: build-load-balancer build-is-port-free build-testmodel

build-load-balancer:
	- g++ -O3 -Wno-unused-result -std=c++17 -I../lib/ LoadBalancer.cpp -o load-balancer -pthread -static-libstdc++ -static-libgcc

build-is-port-free:
	- g++ -O3 -std=c++17 is_port_free.cpp -o is_port_free -static-libstdc++ -static-libgcc

build-testmodel:
	- g++ -O3 -Wno-unused-result -std=c++17 -I../lib/ ../models/testmodel/minimal-server.cpp -o testmodel -pthread -static-libstdc++ -static-libgcc
