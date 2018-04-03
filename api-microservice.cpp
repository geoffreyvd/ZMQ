/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include <zmq.hpp>
#include <iostream>
#include <string>
#include "mongoose.c"
static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;
static int s_sig_num = 0;
static const struct mg_str s_get_method = MG_MK_STR("GET");
static const struct mg_str s_put_method = MG_MK_STR("PUT");
static const struct mg_str s_delele_method = MG_MK_STR("DELETE");
static std::string validVerbs[2] = {"Get", "Post"};

static zmq::context_t context (1);
static zmq::socket_t socket1 (context, ZMQ_REQ);

static void signal_handler(int sig_num) {
  signal(sig_num, signal_handler);
  s_sig_num = sig_num;
}

static int has_prefix(const struct mg_str *uri, const struct mg_str *prefix) {
  return uri->len > prefix->len && memcmp(uri->p, prefix->p, prefix->len) == 0;
}

static bool isValidVerb(struct mg_str route, const struct mg_str *verb){
  return true;
}

static void sendResponse(struct mg_connection *nc, const char *data){
  mg_printf(nc,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n\r\n%s",
                (int) strlen(data), data);
    std::cout << data;
}
static void forwardToBroker(struct mg_connection *nc, std::string serviceID, const mg_str* request){
  zmq::message_t request1 (5);
  memcpy (request1.data (), serviceID.c_str(), strlen(serviceID.c_str()));
  std::cout << "Sending to broker: " << serviceID << std::endl;
  socket1.send (request1);

  //  Get the reply.
  zmq::message_t reply;
  socket1.recv (&reply);

  std::string message = "response fromserviceID: " + serviceID + " response: " +  std::string(static_cast<char*>(reply.data()), reply.size());
  sendResponse(nc, message.c_str());
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  static const struct mg_str apiRouteUser = MG_MK_STR("/api/v1/user");
  static const struct mg_str apiRouteServer = MG_MK_STR("/api/v1/server");
  static const struct mg_str apiRouteMessage = MG_MK_STR("/api/v1/message");

  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case MG_EV_HTTP_REQUEST:
      if (has_prefix(&hm->uri, &apiRouteUser)) {
        if(isValidVerb(apiRouteUser, &hm->method)){
          forwardToBroker(nc, "USER", &hm->method);
          //Todo add data if post verb
        } else {
          mg_printf(nc, "%s",
                    "HTTP/1.0 501 Not Implemented\r\n"
                    "Content-Length: 0\r\n\r\n");
        }
      }
      if (has_prefix(&hm->uri, &apiRouteServer)) {
        if(isValidVerb(apiRouteServer, &hm->method)){
          forwardToBroker(nc, "SERVER", &hm->method);
          //Todo add data if post verb
        } else {
          mg_printf(nc, "%s",
                    "HTTP/1.0 501 Not Implemented\r\n"
                    "Content-Length: 0\r\n\r\n");
        }
      }
      if (has_prefix(&hm->uri, &apiRouteMessage)) {
        if(isValidVerb(apiRouteMessage, &hm->method)){
          forwardToBroker(nc, "MESSAGE", &hm->method);
          //Todo add data if post verb
        } else {
          mg_printf(nc, "%s",
                    "HTTP/1.0 501 Not Implemented\r\n"
                    "Content-Length: 0\r\n\r\n");
        }
      } else {
        mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
      }
      break;
    default:
      break;
  }
}

int main(int argc, char *argv[]) {
  //  Prepare our context and socket

  std::cout << "Connecting to broker" << std::endl;
  socket1.connect ("tcp://localhost:5672");

  struct mg_mgr mgr;
  struct mg_connection *nc;
  int i;

  /* Open listening socket */
  mg_mgr_init(&mgr, NULL);
  nc = mg_bind(&mgr, s_http_port, ev_handler);
  mg_set_protocol_http_websocket(nc);
  s_http_server_opts.document_root = "web_root";

  /* Parse command line arguments */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-D") == 0) {
      mgr.hexdump_file = argv[++i];
    } else if (strcmp(argv[i], "-f") == 0) {
      //s_db_path = argv[++i];
    } else if (strcmp(argv[i], "-r") == 0) {
      s_http_server_opts.document_root = argv[++i];
    }
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  /* Run event loop until signal is received */
  printf("Starting RESTful server on port %s\n", s_http_port);
  while (s_sig_num == 0) {
    mg_mgr_poll(&mgr, 1000);
  }

  /* Cleanup */
  mg_mgr_free(&mgr);

  printf("Exiting on signal %d\n", s_sig_num);

  return 0;
}
