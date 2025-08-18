import sys
import argparse
import pathlib
import numpy as np
import pandas as pd
import sklearn.model_selection
from catboost import CatBoostClassifier, Pool, sum_models
from sklearn.metrics import accuracy_score
from sklearn.datasets import load_svmlight_file
import sklearn.model_selection


# Decrease features dimension (features_size should be passed to features_size in model for predict)
def mod_features(data, features_size):
  transformed_rows = []
  for row_i, row in enumerate(data):
    transformed_row = np.zeros((features_size))
    for col_i, col in enumerate(row):
      if col > 0.000001:
        transformed_row[col_i % features_size] = col
    transformed_rows.append(transformed_row)
  return np.vstack(transformed_rows)


def train(train_svm_file, test_svm_file, features_size):
  train_data_sm, train_label_sm = load_svmlight_file(train_svm_file)
  train_data = train_data_sm.toarray()
  train_label = train_label_sm
  train_data = mod_features(train_data, features_size)

  test_data_sm, test_label_sm = load_svmlight_file(test_svm_file)
  test_data = test_data_sm.toarray()
  test_data = mod_features(test_data, features_size)
  test_label = test_label_sm

  categorical_features_indices = []
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


def train_by_chunks(svm_dir, chunk_size = 1000000, features_size = 1024):
  # read libsvm files and shrink
  cur_train_data_arr = []
  cur_train_label_arr = []
  cur_test_data_arr = []
  cur_test_label_arr = []
  cur_val_data_arr = []  # set for eval metrics between iterations and stop overfitting
  cur_val_label_arr = []
  cur_data_rows = 0

  svm_files = []
  for svm_file in pathlib.Path(svm_dir).glob('**/*.libsvm'):
    svm_files.append(svm_file)
  svm_files.sort()

  max_trees = 100
  use_step_trees = 2
  max_iterations = 1000
  model_saved = False
  sum_model = None

  for train_it in range(max_iterations):
    prev_agg_val_data = None
    prev_agg_val_label = None
    for svm_file_i, svm_file in enumerate(svm_files):
      loaded_data_sm, loaded_label_sm = load_svmlight_file(str(svm_file))
      loaded_data = loaded_data_sm.toarray()
      loaded_label = loaded_label_sm
      adapted_data = mod_features(loaded_data, features_size)
      adapted_label = loaded_label
      cur_data_rows += adapted_data.shape[0]

      # div each file separatly for garantee that test set contains equal rows between rows.
      train_data, test_val_data, train_label, test_val_label = sklearn.model_selection.train_test_split(
        adapted_data,
        adapted_label,
        test_size=0.5,
        shuffle=False,
        random_state=0,
      )

      del loaded_data_sm, loaded_label_sm
      del loaded_data, loaded_label
      del adapted_data, adapted_label

      test_data, val_data, test_label, val_label = sklearn.model_selection.train_test_split(
        test_val_data,
        test_val_label,
        test_size=0.5,
        shuffle=False,
        random_state=0,
      )

      del test_val_data, test_val_label

      cur_train_data_arr.append(train_data)
      cur_train_label_arr.append(train_label)
      cur_test_data_arr.append(test_data)
      cur_test_label_arr.append(test_label)
      cur_val_data_arr.append(val_data)
      cur_val_label_arr.append(val_label)
      if cur_data_rows >= chunk_size:
        break

    agg_train_data = np.vstack(cur_train_data_arr)
    agg_train_label = np.concatenate(cur_train_label_arr)
    agg_test_data = np.vstack(cur_test_data_arr)
    agg_test_label = np.concatenate(cur_test_label_arr)
    agg_val_data = np.vstack(cur_val_data_arr)
    agg_val_label = np.concatenate(cur_val_label_arr)

    #print("train_it=" + str(train_it))
    #print("svm_file_i=" + str(svm_file_i))
    #load_model=('model.cbm' if (train_it > 0 or svm_file_i > 0) else None)
    #print("load_model=" + str(load_model))
    model = CatBoostClassifier(
      iterations=use_step_trees,  # Number of boosting iterations (trees)
      learning_rate=0.1, # Step size shrinkage to prevent overfitting
      depth=6, # Depth of the trees
      loss_function='Logloss', # Loss function for binary classification
      verbose=0 # Suppress training output
    )

    local_train_pool = Pool(agg_train_data, agg_train_label)
    local_eval_pool = Pool(agg_test_data, agg_test_label)
    if sum_model is not None:
      local_train_pool.set_baseline(sum_model.predict(local_train_pool))
      local_eval_pool.set_baseline(sum_model.predict(local_eval_pool))
    model.fit(
      X=local_train_pool,
      eval_set=local_eval_pool,
    )

    # Summarize models by next algo:
    #   concat prev step and current step val datasets
    #   evaluate logloss for all trees in sum model and current model
    #   and choose only best
    # We don't use catboost.sum_models because it increase number of trees ...
    if sum_model is None:
      sum_model = model
    else:
      use_trees_from_step = 2
      sum_model_shrinked = sum_model.copy()
      sum_model_shrinked.shrink(10)
      model_shrinked = model.copy()
      model_shrinked.shrink(10)
      sum_model = sum_models([sum_model_shrinked, model_shrinked])

    model_saved = True
    print("model.tree_count_: " + str(sum_model.tree_count_))
    metrics = sum_model.eval_metrics(
      Pool(agg_val_data, agg_val_label),
      ['Logloss'],
      ntree_start=0,
      ntree_end=0,
      eval_period=model.tree_count_
    )
    logloss = metrics['Logloss'][-1]
    print(str(logloss))
    #sys.exit(1)


parser = argparse.ArgumentParser(description='Train catboost model.')
parser.add_argument('--dir', help='Directory with libsvm files.')
parser.add_argument('--features-size', help='Features size', type=int, default=1024)
parser.add_argument('--chunk-size', help='Features size', type=int, default=1024*1024)
parser.add_argument('--save-model', help='Save model file.')
args = parser.parse_args()

train_by_chunks(
  args.dir,
  chunk_size=args.chunk_size,
  features_size=args.features_size,
)

#model = train(args.train, args.test, args.features_size)
#if args.save_model:
#  model.save_model(args.save_model)
