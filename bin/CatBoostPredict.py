import io
import argparse
import pandas as pd
import sklearn.model_selection
from catboost import CatBoostClassifier, Pool


def predict_csv(csv_file, model = None):
  predict_full = pd.read_csv(csv_file)
  if "label" in predict_full:
    predict_full = predict_full.drop('label', axis=1)

  preds = model.predict_proba(predict_full)
  #print(preds[:, 1])
  return predict_full, preds[:, 1]


parser = argparse.ArgumentParser(description='Train catboost model.')
parser.add_argument('--file', help='Predict csv.')
parser.add_argument('--model', help='Model file.')
args = parser.parse_args()

model = CatBoostClassifier()
model.load_model(args.model)
predict_data, preds = predict_csv(args.file, model)
predict_data.insert(0, 'predicted_label', preds)

output_stream = io.StringIO()
predict_data.to_csv(output_stream, index=False)
csv_string = output_stream.getvalue()
print(csv_string)
