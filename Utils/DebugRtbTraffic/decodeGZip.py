import zlib
import urllib
import sys

def decode_buf(buf) :
  decompressed_data = zlib.decompress(buf, 16 + zlib.MAX_WBITS)
  try :
    print(decompressed_data.decode("ascii") + "\n")
  except :
    pass

buf = bytes()
started_buf = False

while True :
  #data = sys.stdin.buffer.read()
  data = sys.stdin.read()
  if len(data) == 0 :
    break

  while True :
    if started_buf :
      end_pos = data.find(b'POST')
      if end_pos >= 0 :
        buf += data[:end_pos]
        # decode buf
        decode_buf(buf)
        data = data[end_pos:]
        started_buf = False
      else :
        buf += data
        break
    else :
      start_pos = data.find(b'\x0d\x0a\x0d\x0a')
      if start_pos >= 0 :
        buf = bytes()
        data = data[start_pos + 4:]
        started_buf = True
      else :
        break

