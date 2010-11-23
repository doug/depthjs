import zmq
import itertools
import time
import struct
import random
import base64

def main():
  ctx = zmq.Context()
  s = ctx.socket(zmq.PUB)
  bind_to = "tcp://127.0.0.1:14444"
  print bind_to
  s.bind(bind_to)
  types = ["events","image","depth"]
  try:
    #for t in itertools.cycle(["events","image"]):
    for t in itertools.cycle(["image"]):
      msg = ""
      if t == "events":
        msg = """{"type":"TestEvent", "data":{}}"""
      else:
        msg = []
        for x in range(640):
          for y in range(480):
            msg.append(struct.pack("B",random.randint(0,255)))
        msg = base64.b64encode("".join(msg))
      s.send_multipart([t,msg])
      #time.sleep(0.2)
  except KeyboardInterrupt:
    pass

  time.sleep(1)
  print "Done."

if __name__ == "__main__":
  main()
