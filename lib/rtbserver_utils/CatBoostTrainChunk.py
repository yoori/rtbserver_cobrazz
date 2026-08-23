import argparse
import pathlib

from catboost import CatBoostClassifier, Pool


def train_chunk(
    svm_file,
    output_model,
    iterations,
    initial_model=None,
    train_dir=None,
):
  train_pool = Pool('libsvm://' + str(pathlib.Path(svm_file).resolve()))
  model = CatBoostClassifier(
    iterations=iterations,
    learning_rate=0.1,
    depth=6,
    loss_function='Logloss',
    verbose=0,
    train_dir=train_dir,
    use_best_model=False)
  model.fit(
    train_pool,
    init_model=initial_model,
    verbose=True)
  model.save_model(str(pathlib.Path(output_model).resolve()))


def main():
  parser = argparse.ArgumentParser(description='Train one CatBoost chunk.')
  parser.add_argument('--svm-file', required=True)
  parser.add_argument('--output-model', required=True)
  parser.add_argument('--iterations', required=True, type=int)
  parser.add_argument('--initial-model')
  parser.add_argument('--train-dir')
  args = parser.parse_args()
  train_chunk(
    args.svm_file,
    args.output_model,
    args.iterations,
    args.initial_model,
    args.train_dir)


if __name__ == '__main__':
  main()
