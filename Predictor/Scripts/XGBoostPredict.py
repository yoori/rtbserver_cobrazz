#!/usr/bin/python3

import pandas as pd
import xgboost as xgb
from sklearn.metrics import log_loss
from sklearn.datasets import load_svmlight_file
from sklearn.svm import SVC
#import matplotlib.pyplot as plt
import numpy as np
#import graphviz
import argparse

class MatrixReader:
  @staticmethod
  def read(file, labelcol) :
    if file.endswith('.libsvm') :
      data, labels = load_svmlight_file(file) # xgb.DMatrix(args.train_file[0])
    else :
      train_df = pd.read_csv(args.train_file[0], header=0)
      full_matrix = train_df.as_matrix()
      labels = full_matrix[:, labelcol] 
      data = np.delete(full_matrix, args.labelcol, 1)
    return data, labels

parser = argparse.ArgumentParser(description='xgboost predict util.')

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

parser.add_argument(
  '--labelcol',
  type=int,
  nargs='?',
  default=0,
  help='label column index')

args = parser.parse_args()
#print args

# load file from text file, also binary buffer generated by xgboost
train_matrix, train_labels = MatrixReader.read(args.svm_file[0], args.labelcol)
train = xgb.DMatrix(train_matrix, train_labels);

bst = xgb.Booster();
bst.load_model(args.model_file[0]);
preds = bst.predict(train);
for p in preds :
  print(str(p))
