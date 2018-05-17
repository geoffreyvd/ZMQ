This repo implements a zmq broker, worker and client in C. To overcome the discovery problem of the dynamic microservices, we use the dynamic discovery of the broker already inplace, in combination with an api gateway, known as the API gateway pattern (duh). So the api gateway is just another microservice(client) but which implements the translation from the public protocol (http) to internal protocol (ZMQ), and it has a static adress.
So this means that only the api gateway (which can be a microservice) and the broker have a static location (ip and addres), so that the api consumers have an end-point to talk to , for instance mywebsite.com:80 which then converts the http request to a zmq request to the broker.

installation steps done on ubuntu lts 16:
follow https://github.com/zeromq/cppzmq but instead of first build step,
i installed from https://github.com/zeromq/libzmq scroll to deb > follow obs link> click ubuntu > "add installation source" > follow steps
then follow second step off cppzmq

then get files: hwclient , hwserver, c and build from https://github.com/booksbyus/zguide examples/C++
fix permissions with chmod +x for each file, then run:
./build all
and run your program:
./hwclient

lbbBrokerForAPI-MicroService is used in combination with api-microservice
lbbroker2 is a standalone testing ground including clients, workers and the broker
use hwclient and hwserver in combination, can be used in rrbroker2 if used on multiple machines
