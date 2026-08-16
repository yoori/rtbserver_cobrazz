import io
import argparse
import csv
import sys
from catboost import CatBoostClassifier

from rtbserver_utils.CatBoostFeatures import load_catboost_svm


def predict_csv(csv_file, model = None):
  import pandas as pd

  predict_full = pd.read_csv(csv_file)
  if "label" in predict_full:
    predict_full = predict_full.drop('label', axis=1)

  preds = model.predict_proba(predict_full)
  #print(preds[:, 1])
  return predict_full, preds[:, 1]


def predict_svm(svm_file, features_size, model):
  predict_data, _ = load_catboost_svm(svm_file, features_size)
  return model.predict_proba(predict_data)[:, 1]


parser = argparse.ArgumentParser(description='Apply a CatBoost model.')
input_group = parser.add_mutually_exclusive_group(required=True)
input_group.add_argument('--file', help='CSV file to predict.')
input_group.add_argument('--svm', help='LibSVM file to predict.')
parser.add_argument('--model', required=True, help='Model file.')
parser.add_argument('--features-size', type=int, help='CatBoost numeric feature count.')
args = parser.parse_args()

model = CatBoostClassifier()
model.load_model(args.model)
if args.svm:
  if args.features_size is None:
    parser.error('--features-size is required with --svm')
  predictions = predict_svm(args.svm, args.features_size, model)
  writer = csv.writer(sys.stdout)
  writer.writerow(['row_index', 'predicted_ctr'])
  for row_index, prediction in enumerate(predictions):
    writer.writerow([row_index, format(prediction, '.17g')])
else:
  predict_data, predictions = predict_csv(args.file, model)
  predict_data.insert(0, 'predicted_label', predictions)
  output_stream = io.StringIO()
  predict_data.to_csv(output_stream, index=False)
  print(output_stream.getvalue(), end='')
