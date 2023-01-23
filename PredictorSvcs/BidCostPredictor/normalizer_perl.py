import os

def normalize_bid_cost(path_originnal_file, path_save_file):
  file_original = open(path_originnal_file, 'r')
  lines = file_original.readlines()
  file_original.close()

  file_save = open(path_save_file, "a")
  for line in lines:
    vector = line.split(',')
    line = vector[0] + '\t' + vector[1] + '\t' + vector[2] + '\t' + vector[3] + '\t' + vector[4] + '\n'
    file_save.write(line)
  file_save.close()

def normalize_ctr(path_originnal_file, path_save_file):
  file_original = open(path_originnal_file, 'r')
  lines = file_original.readlines()
  file_original.close()

  file_save = open(path_save_file, "a")
  for line in lines:
    vector = line.split(',')
    line = vector[0] + '\t' + vector[1] + '\t' + vector[2] + '\t' + vector[3]
    file_save.write(line)
  file_save.close()

if __name__ == "__main__":
  dirictory = os.path.dirname(os.path.abspath(__file__))

  original_file_path_bid_cost = dirictory + "/" + "perl_bid_cost.csv"
  result_file_path_bid_cost = dirictory + "/" + "perl_normalize_bid_cost.csv"
  normalize_bid_cost(original_file_path_bid_cost, result_file_path_bid_cost)

  original_file_path_ctr = dirictory + "/" + "perl_trivial_ctr.csv"
  result_file_path_ctr = dirictory + "/" + "perl_normalize_trivial_ctr.csv"
  normalize_ctr(original_file_path_ctr, result_file_path_ctr)