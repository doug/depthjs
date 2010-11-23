import zmq
from zmq.eventloop import ioloop

import tornado.httpserver
import tornado.web
import tornado.websocket
from tornado.options import define, options

from random import random
import json
import sys
import os
import multiprocessing as mp
import subprocess

define("port", default=8000, help="port")
define("source", default="localhost", help="data source host")
define("event_port", default=14444, type=int, help="data event port")
define("image_port", default=14445, type=int, help="data image port")
define("depth_port", default=14446, type=int, help="data depth port")

context = zmq.Context()
loop = ioloop.IOLoop.instance()

# ZEROMQ
class Forwarder(object):
  def __init__(self,source,source_port,socktype,loop):
    self.socktype = socktype
    self.datasocket = context.socket(zmq.SUB)
    address = "tcp://%s:%i"%(source,source_port)
    print(address)
    self.datasocket.connect(address)
    self.datasocket.setsockopt(zmq.SUBSCRIBE,'')
    loop.add_handler(self.datasocket, self.read_data, zmq.POLLIN)
  def read_data(self, sock, events):
    if websockets.has_key(self.socktype):
      clients = websockets[self.socktype]
      if clients.empty() == false:
        data = sock.recv()
        for s in clients:
          s.write_message(data)

Forwarder(options.source, options.event_port, "events", loop)
Forwarder(options.source, options.image_port, "image", loop)
Forwarder(options.source, options.depth_port, "depth", loop)

# SOCKETS
websockets = {}

def gen_socket(socktype):
  class DataSocket(tornado.websocket.WebSocketHandler):
    def open(self, *args, **kwargs):
      websockets[socktype] = websockets.get(socktype,[])
      websockets[socktype].append(self)
      print("opened %s" % (socktype,))
    def on_message(self, message):
      print(message)
    def on_close(self):
      websockets[socktype].remove(self)
      print("closed %s" % (socktype,))
  return DataSocket


settings = {
  "static_path": os.path.join(os.path.dirname(__file__), "static"),
  "template_path": os.path.join(os.path.dirname(__file__), "templates"),
}

application = tornado.web.Application([
  (r"/events", gen_socket("events")),
  (r"/image", gen_socket("image")),
  (r"/depth", gen_socket("depth")),
], **settings)

if __name__ == "__main__":
  http_server = tornado.httpserver.HTTPServer(application, io_loop=loop)
  http_server.listen(options.port)
  print "starting %s:%i" % ("localhost", options.port)
  loop.start()
