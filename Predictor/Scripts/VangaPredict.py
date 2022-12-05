#!/usr/bin/python3

import argparse
import bisect
import math
from sklearn.datasets import load_svmlight_file

class DTreeBranchHolder :
  def __init__(self, feature_id, yes_tree_id, no_tree_id) :
    self.feature_id = feature_id
    self.yes_tree_id = yes_tree_id
    self.no_tree_id = no_tree_id

class DTreeHolder :
  def __init__(self, delta, branches) :
    self.delta = delta
    self.branches = branches
    self.resolved_node = None

class DTreeBranch :
  def __init__(self, feature_id, yes_tree, no_tree) :
    self.feature_id = feature_id
    self.yes_tree = yes_tree
    self.no_tree = no_tree

class DTree :
  def __init__(self, delta) :
    self.delta = delta
    self.branches = list()

  @staticmethod
  def load(file) :
    nodes = dict()
    head = file.readline()
    #print head
    root_tree_id = 0
    for line in file :
      #print line
      line = line.rstrip('\r\n')
      if not line :
        break
      tree_id_str, delta_str, branches_str = line.split("\t")
      tree_id = int(tree_id_str)
      if root_tree_id == 0 :
        root_tree_id = tree_id
      delta = float(delta_str)
      #print '<' + branches_str + '>' 
      branches = list()
      if branches_str :
        branche_strs = branches_str.split("|")
        for branch_str in branche_strs :
          feature_id_str, yes_tree_id_str, no_tree_id_str = branch_str.split(':')
          feature_id = int(feature_id_str)
          yes_tree_id = int(yes_tree_id_str)
          no_tree_id = int(no_tree_id_str)
          branches.append(DTreeBranchHolder(feature_id, yes_tree_id, no_tree_id))
      nodes[tree_id] = DTreeHolder(delta, branches)
    for tree_id, node_holder in nodes.iteritems() :
      node_holder.resolved_node = DTree(node_holder.delta)
    for tree_id, node_holder in nodes.iteritems() :
      for branch_holder in node_holder.branches :
        yes_tree = nodes[branch_holder.yes_tree_id].resolved_node
        no_tree = nodes[branch_holder.no_tree_id].resolved_node
        node_holder.resolved_node.branches.append(
          DTreeBranch(branch_holder.feature_id, yes_tree, no_tree))
    #print root_tree_id
    return nodes[root_tree_id].resolved_node

  def predict(self, features) :
    res = self.delta
    for branch in self.branches :
      found_index = bisect.bisect_left(features, branch.feature_id)
      #print "-----------"
      #print features
      #print branch.feature_id
      #print found_index
      if found_index < len(features) and features[found_index] == branch.feature_id :
        res += branch.yes_tree.predict(features)
      else :
        res += branch.no_tree.predict(features)
    return res

# main
parser = argparse.ArgumentParser(description='vanga predict util.')

parser.add_argument(
  'model_file',
  metavar='<model file>',
  type=str,
  nargs=1,
  help='model out file')

parser.add_argument(
  'svm_file',
  metavar='<svm file>',
  type=str,
  nargs=1,
  help='svm file')

args = parser.parse_args()

predictor = DTree.load(open(args.model_file[0], 'r'));

data, labels = load_svmlight_file(args.svm_file[0])

for row in data :
  #l = list()
  #for element in row :
  #  print "element:"
  #  print element
  #print row.nonzero()[1]
  #print "=============="
  # row sorted
  pred = predictor.predict([x + 1 for x in row.nonzero()[1]])
  print 1.0 / (1.0 + math.exp(-pred))
