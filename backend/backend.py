import zmq
from zmq.eventloop import ioloop

import tornado.httpserver
import tornado.web
import tornado.websocket
from tornado.options import define, options, parse_command_line

from random import random
import json
import sys
import os
import multiprocessing as mp
import subprocess
import time
import base64

define("port", default=8000, help="port")
define("source", default="tcp://127.0.0.1:14444", help="data source host")
define("darkperiod", default=0.5, help="darkperiod after which you ignore events")
define("imgdarkperiod", default=0.5, help="darkperiod after which you ignore images")

# ZEROMQ
class Forwarder(object):
  def __init__(self,source,loop):
    self.lastevent = time.time()
    self.lastimg = time.time()
    self.datasocket = context.socket(zmq.SUB)
    self.datasocket.connect(source)
    self.datasocket.setsockopt(zmq.SUBSCRIBE,'')
    loop.add_handler(self.datasocket, self.read_data, zmq.POLLIN)
  def read_data(self, sock, events):
    t, data = sock.recv_multipart()
    print("got a %s" % (t,))
    clients = websockets.get(t,[])
    if len(clients) != 0:
      forward = True
      now = time.time()
      if t == "event":
        datajson = json.loads(data)
        dtype = datajson["type"]
        print(dtype)
        if dtype != "move" and (now - self.lastevent) < options.darkperiod:
          self.lastevent = now
          print("don't forward")
          forward = False
      if t in ["image", "depth"] and (now - self.lastimg) > options.imgdarkperiod:
        print("IMAGEEE!!!!!!!")
        self.lastimg = now
        data = base64.b64encode(data)
        forward = False
      #else:
        #print("IMAGE BUt tIME is wrong")
        #print((now - self.lastimg))
        #forward = False
      #if t in ["image", "depth"]:
        #forward = False
      if forward == True:
        print("forwarding")
        for s in clients:
          s.write_message(data)

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
  (r"/event", gen_socket("event")),
  (r"/image", gen_socket("image")),
  (r"/depth", gen_socket("depth")),
], **settings)

if __name__ == "__main__":
  parse_command_line()
  context = zmq.Context()
  loop = ioloop.IOLoop.instance()
  Forwarder(options.source, loop)
  http_server = tornado.httpserver.HTTPServer(application, io_loop=loop)
  http_server.listen(int(options.port))
  print "starting %s:%s" % ("localhost", options.port)
  loop.start()
