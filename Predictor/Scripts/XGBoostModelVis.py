import pandas as pd
import xgboost as xgb
from sklearn.metrics import log_loss
import matplotlib.pyplot as plt
import numpy as np
import graphviz
import argparse
from matplotlib.backends.backend_pdf import PdfPages

parser = argparse.ArgumentParser(description='xgboost model visualize util.')

parser.add_argument(
  'out_file',
  metavar='<out file>',
  type=str,
  nargs=1,
  help='vis out file')

parser.add_argument(
  'model_file',
  metavar='<model file>',
  type=str,
  nargs='*',
  help='model file')

args = parser.parse_args()

if args.out_file[0].endswith('pdf') :
  pp = PdfPages(args.out_file[0])
  for model_file in args.model_file :
    print "model_file: " + model_file
    fig = plt.figure()
    bst = xgb.Booster({'nthread':1})
    bst.load_model(model_file)
    axes = fig.add_axes([0,0,1,1])
    xgb.plot_tree(bst, ax = axes, num_trees=0) #, num_trees=10)
    pp.savefig(fig)
    fig.close()
    fig = None
  pp.close()
else :
  model_file = args.model_file[0]
  fig = plt.figure()
  bst = xgb.Booster({'nthread':1})
  bst.load_model(model_file)
  axes = fig.add_axes([0,0,1,1])
  xgb.plot_tree(bst, ax = axes, num_trees=0) #, num_trees=10)
  fig.savefig(args.out_file[0], dpi=1000)
  print "save into '" + args.out_file[0] + "' model from '" + model_file + "'"
#fig.savefig(args.out_file[0], dpi=1000) # save into png

plt.close()
