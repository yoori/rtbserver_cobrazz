import argparse
import pandas as pd
import sklearn.model_selection
from catboost import CatBoostClassifier, Pool
from sklearn.metrics import accuracy_score


def train(train_csv_file, test_csv_file):
  train_full = pd.read_csv(train_csv_file)
  train_data = train_full.drop('label', axis=1)
  train_label = train_full['label']

  test_full = pd.read_csv(test_csv_file)
  test_data = test_full.drop('label', axis=1)
  test_label = test_full['label']

  categorical_features_indices = []
  for col_i, col_name in enumerate(train_data):
    if col_name.endswith("(cat)"):
      categorical_features_indices.append(col_i)

  train_pool = Pool(train_data, train_label, cat_features=categorical_features_indices)
  test_pool = Pool(test_data, test_label, cat_features=categorical_features_indices)

  # Step 2: Initialize and train the CatBoost model
  model = CatBoostClassifier(
    iterations=100,  # Number of boosting iterations (trees)
    learning_rate=0.1, # Step size shrinkage to prevent overfitting
    depth=6,          # Depth of the trees
    loss_function='Logloss', # Loss function for binary classification
    verbose=0         # Suppress training output
  )

  model.fit(train_pool)
  return model


parser = argparse.ArgumentParser(description='Train catboost model.')
parser.add_argument('--train', help='Train csv.')
parser.add_argument('--test', help='Test csv.')
parser.add_argument('--save-model', help='Save model file.')
args = parser.parse_args()

model = train(args.train, args.test)
if args.save_model:
  model.save_model(args.save_model)
