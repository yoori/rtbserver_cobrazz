import argparse
import sklearn.model_selection
from catboost import CatBoostClassifier, Pool

from rtbserver_utils.CatBoostFeatures import load_catboost_svm


class CatBoostTrainer(object):
  features_size: int = None

  def __init__(self, features_dimension = 24):
    self.features_size = 1 << features_dimension

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
    full_svm, full_label = load_catboost_svm(svm_file, self.features_size)
    train_data_sm, test_data_sm, train_label_sm, test_label_sm = (
      sklearn.model_selection.train_test_split(
        full_svm,
        full_label,
        test_size=0.2,
        random_state=42))
    return self.train_prepared_(
      train_data_sm,
      test_data_sm,
      train_label_sm,
      test_label_sm)

  def train(self, train_svm_file, test_svm_file):
    train_data_sm, train_label_sm = load_catboost_svm(
      train_svm_file,
      self.features_size)
    test_data_sm, test_label_sm = load_catboost_svm(
      test_svm_file,
      self.features_size)
    return self.train_prepared_(
      train_data_sm,
      test_data_sm,
      train_label_sm,
      test_label_sm)

  def train_prepared_(
      self,
      train_data,
      test_data,
      train_label,
      test_label,
  ):
    print("Train set: " + str(train_data.shape))
    print("Test set: " + str(test_data.shape))

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
