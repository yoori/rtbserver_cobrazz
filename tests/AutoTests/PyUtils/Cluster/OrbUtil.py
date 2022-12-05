
import struct

FRACTION=8
FRACTION_DENOMINATOR=10**FRACTION
DECIMAL_SIZE = 18

def time2orb( time = None):
  sec, mcs = 0, 0
  if time:
    sec, mcs = time
  if not sec:
    sec = 0
  if not mcs:
    mcs = 0
  return struct.pack("<ii", sec, mcs)

def orb2time ( v ):
  sec, mcs = struct.unpack("<ii", v)
  if not (sec or mcs): return None
  return (sec, mcs)

def float2decimal( v ):
  sign = 0
  if v < 0:
    sign = 1
    v = (-1) * v
  buffer = "%010d%08d" % (int(v), int(v * FRACTION_DENOMINATOR % FRACTION_DENOMINATOR))
  val = 0
  for digit in buffer:
    val = val * 10 + (ord(digit) - ord('0'))
  return val, sign

def decimal2float( v, sign ):
  integer = fraction = ""
  for i in range(DECIMAL_SIZE):
    c = chr( (v % 10) + ord('0'))
    if (i < FRACTION):
      fraction = c + fraction 
    else:
      integer = c + integer 
    v = v / 10
  buf = "%s.%s" % (integer, fraction)
  val = float(buf)
  if sign:
    return (-1) * val
  return val

def decimal2orb( v ):
  val, sign = float2decimal(v)
  buffer = struct.pack("<QB", val, sign)
  return buffer

def orb2decimal( v ):
  val, sign = struct.unpack("<QB", v)
  return decimal2float(val, sign)
