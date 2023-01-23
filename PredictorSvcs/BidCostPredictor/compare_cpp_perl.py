import os
from decimal import Decimal

def test_bid_cost(path_perl, path_cpp):
  file_perl = open(path_perl, 'r')
  lines_perl = file_perl.readlines()
  lines_perl.sort()
  file_perl.close()

  file_perl = open(path_cpp, 'r')
  lines_cpp = file_perl.readlines()
  lines_cpp.sort()
  file_perl.close()

  if (len(lines_perl) != len(lines_cpp)):
    print("Files did not match")
    return False

  size = len(lines_perl)
  for i in range(0, size - 1):
    line_cpp = lines_cpp[i]
    vector_cpp = line_cpp.split('\t')
    line_perl = lines_perl[i]
    vector_perl = line_perl.split('\t')
    if vector_cpp[0] != vector_perl[0] or vector_cpp[1] != vector_perl[1] or Decimal(vector_cpp[2]) != Decimal(vector_perl[2]) or Decimal(vector_cpp[3]) != Decimal(vector_perl[3]) or Decimal(vector_cpp[4]) != Decimal(vector_perl[4]):
      print(line_cpp)
      print(line_perl)
      print("Not correct value")
      #return False

  print("BID_COST_MODEL ALL OK")
  return True

def test_ctr(path_perl, path_cpp):
  file_perl = open(path_perl, 'r')
  lines_perl = file_perl.readlines()
  lines_perl.sort()
  file_perl.close()

  file_perl = open(path_cpp, 'r')
  lines_cpp = file_perl.readlines()
  lines_cpp.sort()
  file_perl.close()

  if (len(lines_perl) != len(lines_cpp)):
    print("Files did not match")
    return False

  size = len(lines_perl)
  for i in range(0, size - 1):
    line_cpp = lines_cpp[i]
    vector_cpp = line_cpp.split('\t')
    line_perl = lines_perl[i]
    vector_perl = line_perl.split('\t')
    if vector_cpp[0] != vector_perl[0] or vector_cpp[1] != vector_perl[1] or vector_cpp[2] != vector_perl[2] or vector_cpp[3] != vector_perl[3]:
      print(line_cpp)
      print(line_perl)
      print("Not correct value")
      return False

  print("CTR_MODEL ALL OK")
  return True

if __name__ == "__main__":
  dirictory = os.path.dirname(os.path.abspath(__file__))

  path_perl_bid_cost = dirictory + "/" + "perl_normalize_bid_cost.csv"
  path_cpp_bid_cost = dirictory + "/" + "cpp_bid_cost.csv"
  test_bid_cost(path_perl_bid_cost, path_cpp_bid_cost)

  path_perl_ctr = dirictory + "/" + "perl_normalize_trivial_ctr.csv"
  path_cpp_ctr = dirictory + "/" + "cpp_trivial_ctr.csv"
  test_ctr(path_perl_ctr, path_cpp_ctr)