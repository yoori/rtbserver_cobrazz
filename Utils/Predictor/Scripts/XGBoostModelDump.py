import pandas as pd
import xgboost as xgb
from sklearn.metrics import log_loss
import matplotlib.pyplot as plt
import numpy as np
import graphviz
import argparse
from matplotlib.backends.backend_pdf import PdfPages

parser = argparse.ArgumentParser(description='xgboost model dump util.')

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
  nargs=1,
  help='model file')

parser.add_argument(
  'fmap_file',
  metavar='<fmap file>',
  type=str,
  nargs=1,
  help='fmap out file')

args = parser.parse_args()

model_file = args.model_file[0]
bst = xgb.Booster({'nthread':1})
bst.load_model(model_file)

if len(args.fmap_file) > 0 :
  bst.dump_model(args.out_file[0], args.fmap_file[0], with_stats = True)
else :
  bst.dump_model(args.out_file[0], with_stats = True)
