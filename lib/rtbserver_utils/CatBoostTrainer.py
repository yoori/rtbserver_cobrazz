import sys
import argparse
import pathlib
import numpy as np
import pandas as pd
import sklearn.model_selection
import scipy.sparse
from catboost import CatBoostClassifier, Pool, sum_models
from sklearn.metrics import accuracy_score
from sklearn.datasets import load_svmlight_file
import sklearn.model_selection


class CatBoostTrainer(object):
  features_size: int = None

  def __init__(self, features_dimension = 24):
    self.features_size = 1 << features_dimension

  # Decrease features dimension (features_size should be passed to features_size in model for predict)
  @staticmethod
  def numpy_matrix_mod_features(data, features_size=1024):
    transformed_rows = []
    for row_i, row in enumerate(data):
      transformed_row = np.zeros((self.features_size))
      for col_i, col in enumerate(row):
        sys.exit(1)
        if col > 0.000001:
          transformed_row[col_i % features_size] = col
      transformed_rows.append(transformed_row)
    return np.vstack(transformed_rows)

  @staticmethod
  def csr_matrix_mod_features(data, features_size=1024):
    res_matrix = scipy.sparse.lil_matrix((data.shape[0], features_size))
    nonzero_rows, nonzero_cols = data.nonzero()
    l = len(nonzero_rows)
    for i, (row_i, col_i) in enumerate(zip(nonzero_rows, nonzero_cols)):
      res_matrix[row_i, col_i % features_size] = data[row_i, col_i]
      if i % 100000 == 0:
        print("Processed " + str(i) + "/" + str(l))
    return scipy.sparse.csr_matrix(res_matrix)

  def mod_features(self, data):
    if isinstance(data, np.ndarray):
      return CatBoostTrainer.numpy_matrix_mod_features(data, features_size=self.features_size)
    elif scipy.sparse.isspmatrix_csr(data):
      return CatBoostTrainer.csr_matrix_mod_features(data, features_size=self.features_size)
    else:
      raise ValueError("Can't mod features for type: " + str(type(data)))

  def train_on_pools(self, train_pool: Pool, test_pool: Pool):
    # Step 2: Initialize and train the CatBoost model
    model = CatBoostClassifier(
      iterations=100,  # Number of boosting iterations (trees)
      learning_rate=0.1, # Step size shrinkage to prevent overfitting
      depth=6,          # Depth of the trees
      loss_function='Logloss', # Loss function for binary classification
      verbose=0         # Suppress training output
    )

    print("To fit")
    model.fit(train_pool, verbose=True)
    return model

  def split_and_train(self, svm_file):
    full_svm, full_label = load_svmlight_file(svm_file)
    train_data_sm, test_data_sm, train_label_sm, test_label_sm = sklearn.model_selection.train_test_split(
      full_svm,
      full_label,
      test_size=0.2,
      random_state=42
    )
    self.train_(train_data_sm, test_data_sm, train_label_sm, test_label_sm)

  def train(self, train_svm_file, test_svm_file):
    train_data_sm, train_label_sm = load_svmlight_file(train_svm_file)
    test_data_sm, test_label_sm = load_svmlight_file(test_svm_file)
    return self.train_(train_data_sm, train_label_sm, test_data_sm, test_label_sm)

  def train_(self, train_data_sm, test_data_sm, train_label_sm, test_label_sm):
    print("To prepare train set: " + str(train_data_sm.shape))
    # train_data = train_data_sm.toarray()
    train_data = train_data_sm
    train_label = train_label_sm
    train_data = self.mod_features(train_data)
    print("From prepare train set")

    # test_data = test_data_sm.toarray()
    print("To prepare test set")
    test_data = test_data_sm
    test_data = self.mod_features(test_data)
    test_label = test_label_sm
    print("From prepare test set")

    categorical_features_indices = []
    train_pool = Pool(train_data, train_label, cat_features=categorical_features_indices)
    test_pool = Pool(test_data, test_label, cat_features=categorical_features_indices)
    return self.train_on_pools(train_pool, test_pool)


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Train catboost model.')
  parser.add_argument('--train-file', help='Train libsvm.')
  parser.add_argument('--test-file', help='Test libsvm.')
  parser.add_argument('--features-dimension', help='Features size', type=int, default=24)
  parser.add_argument('--save-model', help='Save model file.')
  args = parser.parse_args()

  trainer = CatBoostTrainer(features_dimension=args.features_dimension)
  model = trainer.train(args.train_file, args.test_file)
  model.save_model(args.save_model)
