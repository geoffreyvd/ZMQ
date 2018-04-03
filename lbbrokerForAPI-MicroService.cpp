//  Least-recently used (LRU) queue device
//  Clients and workers are shown here in-process
// ./c -p -l -lzmq -std=c++11 lbbrokerForAPI-MicroService

#include "zhelpers.hpp"
#include <pthread.h>
#include <queue>
#include <iostream>
#include <fstream>
#include <chrono>
using namespace std;

typedef std::chrono::high_resolution_clock Clock;
int amountOfRequestsPerClient = 5000;
int amountOfClients = 50;
int amountOfWorkers = 1;

//  Basic request-reply client using REQ socket
//

//  Worker using REQ socket to do LRU routing
//
static void *
worker_thread(void *arg) {
    zmq::context_t context(1);
    zmq::socket_t worker(context, ZMQ_REQ);

    #if (true)
        s_set_id(worker);
        worker.connect("tcp://localhost:5673"); // backend
    #else
        s_set_id(worker);
        worker.connect("ipc://backend.ipc");
    #endif

    //  Tell backend we're ready for work
    s_send(worker, "READY");
    // ofstream myfile;
    // myfile.open ("clientOutput" + std::to_string((intptr_t)arg) + ".txt");

    while (1) {
        //  Read and save all frames until we get an empty frame
        //  In this example there is only 1 but it could be more
        std::string address = s_recv(worker);
        {
            std::string empty = s_recv(worker);
            assert(empty.size() == 0);
        }

        //  Get request, send reply
        std::string request = s_recv(worker);
        cout << "Worker " << (intptr_t)arg << " received: " << request << std::endl;

        s_sendmore(worker, address);
        s_sendmore(worker, "");
        s_send(worker, "OK");
    }
    // myfile.close();
    return (NULL);
}

int main(int argc, char *argv[])
{
    cout << "booting broker with:" << std::endl;
    cout << "# of clients: " << amountOfClients << std::endl;
    cout << "# of reqeusts per client: " << amountOfRequestsPerClient << std::endl;
    cout << "# of workers: " << amountOfWorkers << std::endl;
    auto t1 = Clock::now();

    //  Prepare our context and sockets
    zmq::context_t context(1);
    zmq::socket_t frontend(context, ZMQ_ROUTER);
    zmq::socket_t backend(context, ZMQ_ROUTER);

    #if (true)
        frontend.bind("tcp://*:5672"); // frontend
        backend.bind("tcp://*:5673"); // backend
    #else
        frontend.bind("ipc://frontend.ipc");
        backend.bind("ipc://backend.ipc");
    #endif

    int worker_nbr;
    for (worker_nbr = 0; worker_nbr < amountOfWorkers; worker_nbr++) {
        pthread_t worker;
        pthread_create(&worker, NULL, worker_thread, (void *)(intptr_t)worker_nbr);
    }

    //  Logic of LRU loop
    //  - Poll backend always, frontend only if 1+ worker ready
    //  - If worker replies, queue worker as ready and forward reply
    //    to client if necessary
    //  - If client requests, pop next worker and send request to it
    //
    //  A very simple queue structure with known max size
    std::queue<std::string> worker_queue;

    // ofstream myfile;
    // myfile.open ("brokerOutput.txt");

    while (1) {

        //  Initialize poll set
        zmq::pollitem_t items[] = {
            //  Always poll for worker activity on backend
                { backend, 0, ZMQ_POLLIN, 0 },
                //  Poll front-end only if we have available workers
                { frontend, 0, ZMQ_POLLIN, 0 }
        };
        if (worker_queue.size())
            zmq::poll(&items[0], 2, -1);
        else
            zmq::poll(&items[0], 1, -1);

        //  Handle worker activity on backend
        if (items[0].revents & ZMQ_POLLIN) {

            //  Queue worker address for LRU routing
            worker_queue.push(s_recv(backend));

            {
                //  Second frame is empty
                std::string empty = s_recv(backend);
                assert(empty.size() == 0);
            }

            //  Third frame is READY or else a client reply address
            std::string client_addr = s_recv(backend);

            //  If client reply, send rest back to frontend
            if (client_addr.compare("READY") != 0) {

                    {
                        std::string empty = s_recv(backend);
                        assert(empty.size() == 0);
                    }

                std::string reply = s_recv(backend);
                s_sendmore(frontend, client_addr);
                s_sendmore(frontend, "");
                s_send(frontend, reply);

                //log backend response
                // myfile << "RES forward_to_client_addr = " << client_addr << " ,payload = " << reply<< std::endl;
                //static breaek condition:for each client there is a static amount of requests
                // if (--client_nbr_requests == 0)
                //     break;
            }
        }
        if (items[1].revents & ZMQ_POLLIN) {

            //  Now get next client request, route to LRU worker
            //  Client request is [address][empty][request]
            std::string client_addr = s_recv(frontend);

            {
                std::string empty = s_recv(frontend);
                assert(empty.size() == 0);
            }

            std::string request = s_recv(frontend);

            std::string worker_addr = worker_queue.front();//worker_queue [0];
            worker_queue.pop();

            s_sendmore(backend, worker_addr);
            s_sendmore(backend, "");
            s_sendmore(backend, client_addr);
            s_sendmore(backend, "");
            s_send(backend, request);

            //log frontend reqeust
            // myfile << "REQ client_addr = " << client_addr << "forward_to_worker_addr = "
            //  << worker_addr << " ,payload = " << request << std::endl;
        }
    }

    auto t2 = Clock::now();
    std::cout << "Duration: "
              << (std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()) / 1000000
              << " miliseconds" << std::endl;
    return 0;
}
