import zmq
from zmq.eventloop import ioloop

import tornado.httpserver
import tornado.web
import tornado.websocket
import tornado.options import define, options

from random import random
import json
import sys
import os
import multiprocessing as mp
import subprocess

define("port", default=8000, help="port")
define("source", default="localhost", help="data source host")
define("event_port", default="14444", help="data event port")
define("image_port", default="14445", help="data image port")
define("depth_port", default="14446", help="data depth port")

loop = ioloop.IOLoop.instance()

# ZEROMQ
class EventReader(object):
  def __init__(self,chan,context,loop):
    self.chan = chan
    self.datasocket = context.socket(zmq.SUB)
    self.datasocket.connect("%s:%i"%(DATA_ADDRESS,DATA_PORT+chan))
    self.datasocket.setsockopt(zmq.SUBSCRIBE,'')
    loop.add_handler(self.datasocket, self.read_data, zmq.POLLIN)
  def read_data(self, sock, events):
    if websockets.has_key(self.chan):
      dp = data_pb2.DataPacket()
      dp.ParseFromString(sock.recv())
      data = downsample(dp.samples,DOWNSAMPLE)
      jsonmsg = json.dumps(data)
      for s in websockets[self.chan]:
        s.write_message(jsonmsg)

class ImageReader(object):
  def __init__(self,chan,context,loop):
    self.chan = chan
    self.datasocket = context.socket(zmq.SUB)
    self.datasocket.connect("%s:%i"%(DATA_ADDRESS,DATA_PORT+chan))
    self.datasocket.setsockopt(zmq.SUBSCRIBE,'')
    loop.add_handler(self.datasocket, self.read_data, zmq.POLLIN)
  def read_data(self, sock, events):
    if websockets.has_key(self.chan):
      dp = data_pb2.DataPacket()
      dp.ParseFromString(sock.recv())
      data = downsample(dp.samples,DOWNSAMPLE)
      jsonmsg = json.dumps(data)
      for s in websockets[self.chan]:
        s.write_message(jsonmsg)

class Forwarder(object):
  def __init__(self,chan,context,loop):
    self.chan = chan
    self.datasocket = context.socket(zmq.SUB)
    self.datasocket.connect("%s:%i"%(DATA_ADDRESS,DATA_PORT+chan))
    self.datasocket.setsockopt(zmq.SUBSCRIBE,'')
    loop.add_handler(self.datasocket, self.read_data, zmq.POLLIN)
  def read_data(self, sock, events):
    if websockets.has_key(self.chan):
      dp = data_pb2.DataPacket()
      dp.ParseFromString(sock.recv())
      data = downsample(dp.samples,DOWNSAMPLE)
      jsonmsg = json.dumps(data)
      for s in websockets[self.chan]:
        s.write_message(jsonmsg)



context = zmq.Context()
readers = {}
for chan in range(NUM_CHANNELS):
  dr = DataReader(chan,context,loop)
  readers[chan] = dr



# SOCKETS
websockets = {}

class ImageSocket(tornado.websocket.WebSocketHandler):
  def open(self, *args, **kwargs):
    print("open")
  def on_message(self, message):
    print(message)
  def on_close(self):
    print("close")

class DepthSocket(tornado.websocket.WebSocketHandler):
  def open(self, *args, **kwargs):
    print("open")
  def on_message(self, message):
    print(message)
  def on_close(self):
    print("close")

class EventSocket(tornado.websocket.WebSocketHandler):
  def open(self, *args, **kwargs):
    print("open")
  def on_message(self, message):
    print(message)
  def on_close(self):
    print("close")

# watch files for events when modules are updated

settings = {
  "static_path": os.path.join(os.path.dirname(__file__), "static"),
  "template_path": os.path.join(os.path.dirname(__file__), "templates"),
}

application = tornado.web.Application([
  (r"/events", EventSocket),
  (r"/image", ImageSocket),
  (r"/depth", DepthSocket),
], **settings)

if __name__ == "__main__":
  http_server = tornado.httpserver.HTTPServer(application, io_loop=loop)
  http_server.listen(options.port)
  print "starting localhost:"+options.port
  loop.start()
