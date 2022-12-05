#!/usr/bin/env python

import operator
import pandas as pd
import xgboost as xgb
from sklearn.metrics import log_loss
import matplotlib.pyplot as plt
import numpy as np
import graphviz
import argparse
from matplotlib.backends.backend_pdf import PdfPages

parser = argparse.ArgumentParser(description='xgboost model importance util.')

parser.add_argument(
  'out_file',
  metavar='<out file>',
  type=str,
  nargs=1,
  help="vis out file or 'batch' for batch mode")

parser.add_argument(
  'model_file',
  metavar='<model file>',
  type=str,
  nargs=1,
  help='model file')

parser.add_argument(
  'dict',
  metavar='<dictionary file>',
  type=str,
  nargs='?',
  help='dictionary file in format "<hash>,<>"')

parser.add_argument(
  '--limit',
  dest='limit',
  type=int,
  nargs='?',
  default=30,
  help='features limit')

args = parser.parse_args()

# load model
#print "model: '" + args.model_file[0] + "'\n"
model_file = args.model_file[0]
bst = xgb.Booster({'nthread':1})
bst.load_model(model_file)

# save importance graph
importance = bst.get_fscore()

if not importance:
  print >> sys.stderr, 'Got invalid model (empty features)!'
  exit(1)
  
importance = sorted(importance.items(), key=operator.itemgetter(1),
                    reverse=True)
sum = sum(map(lambda x: x[1], importance))

if args.limit:
  importance=importance[0:args.limit]

# replace
if args.dict:
  pd.set_option('display.max_rows', 500)
  pd.set_option('display.width', 1000)
  pd.set_option('display.expand_frame_repr', True)
  dict = pd.read_csv(args.dict, header=None)
  dict_map = {}

  for row in dict.values :
    dict_map[int(row[0])] = row[1]

  for i in range(len(importance)):
    importance[i] = (dict_map[int(importance[i][0][1:]) + 1], importance[i][1])

if args.out_file[0] == 'batch':
  for (f, s) in importance:
    print "%s, %f" % (f, float(s) / sum)
else:
  importance.reverse()

  features = map(lambda x: x[0], importance)
  
  df = pd.DataFrame(
    {'feature': range(len(importance)),
     'fscore': map(lambda x: float(x[1]) / sum, importance) } ) 
  
  df = df.set_index('feature')
  
  ax =df.plot(
    kind='barh',
    legend = False,
    color='#edd77e',
    title='XGBoost Feature Importance',
    figsize=(20,10))
  
  ax.set_alpha(0.8)
  ax.set_xlabel('relative importance', fontsize=16)

  plt.rc('font', family='DejaVu Sans')

  label_idx = 0
  for i in ax.patches:
    rx, ry = i.get_xy()
    lx = rx + 0.001
    cy = ry + i.get_height()/2.0
    ax.annotate(features[label_idx].decode("utf8"), (lx, cy), \
      ha='left', va='center', fontsize=10)
    label_idx+=1

  plt.gcf().savefig(args.out_file[0])
