steps done on ubuntu lts 16:
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
