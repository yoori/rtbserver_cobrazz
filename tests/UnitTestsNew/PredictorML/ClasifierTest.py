import argparse
import os
import pandas as pd
import sys
from catboost import CatBoostClassifier
from catboost import Pool
from pathlib import Path
from sklearn.model_selection import train_test_split

class MLApplication:
  csv_file_path = ""
  model_file_path = ""
  file_probability_path = ""
  iterations = 0
  learning_rate = 0
  early_stopping_rounds = 0

  def __init__(
      self, csv_file_path, model_file_path, file_probability_path,
      iterations, learning_rate, early_stopping_rounds):
    self.csv_file_path = csv_file_path
    self.model_file_path = model_file_path
    self.iterations = iterations
    self.learning_rate = learning_rate
    self.early_stopping_rounds = early_stopping_rounds
    self.file_probability_path = file_probability_path

    model_directory = Path(model_file_path).parent
    if not os.path.exists(model_directory):
      raise Exception(
        "Not existing model directory: {}"
        .format(model_directory))

  def create_file(
      self, tag_to_label, number_record,
      feature_cat_number, feature_number_number):
    if feature_cat_number == 0:
      raise Exception(
        'Number of category features '
        'must be larger then 0')

    label_tag_list = list()
    for tag, label in tag_to_label.items():
      print("tag={}->label{}".format(tag, label))
      label_tag_list.append((label, tag))

    print("Number record : " + str(number_record))
    print("Begin creating data of file=" + self.csv_file_path + "...")
    feature_number = feature_cat_number + feature_number_number
    number_zero_feature = feature_number - 1
    tag_size = len(label_tag_list)

    with open(self.csv_file_path, "w") as file:
      file.write("label")
      for i in range(feature_cat_number):
        file.write("," + "feature_cat" + str(i + 1))
      for i in range(feature_number_number):
        file.write("," + "feature_number_number" + str(i + 1))
      file.write("\n")
      for i in range(number_record):
        index = i % tag_size
        label_tag = label_tag_list[index]
        file.write(str(label_tag[0]) + "," + str(label_tag[1]))
        for j in range(number_zero_feature):
          file.write(",0")
        file.write("\n")
    print("File created successfully")

  def learn(self):
    print("Start learning...")
    train_df = pd.read_csv(self.csv_file_path)
    y = train_df.label
    x = train_df.drop('label', axis = 1)
    cat_features = list(range(0, feature_cat_number))
    print("Categories indexes: {}".format(cat_features))
    labels = sorted(set(y))
    print('Labels: {}'.format(labels))
    tags = sorted(set(x.iloc[:, 0]))
    print('Tags: {}'.format(tags))

    pool = Pool(data=x, label=y, cat_features=cat_features)
    print('Column names: {}\n\n'.format(pool.get_feature_names()))

    x_train, x_validation, y_train, y_validation = (
      train_test_split(x, y, train_size=0.8, random_state=1))

    is_binary_classification = True
    loss_function = ""
    if len(labels) == 2:
      is_binary_classification = True
      loss_function = "Logloss"
    else:
      is_binary_classification = False
      loss_function = "MultiClass"

    train_dir = Path(os.path.abspath(__file__)).parents[3].absolute()
    train_dir = train_dir / "build/train_ml_info"
    model = CatBoostClassifier(
      iterations = self.iterations,
      learning_rate = self.learning_rate,
      early_stopping_rounds = self.early_stopping_rounds,
      loss_function = loss_function,
      random_seed = 777,
      custom_loss=['AUC', 'Accuracy'],
      train_dir = train_dir)
    model.fit(
      x_train, y_train,
      cat_features=cat_features,
      eval_set=(x_validation, y_validation),
      verbose=100,
      plot=False)

    print('Model is fitted: {}'.format(model.is_fitted()))
    print('Model params: {}'.format(model.get_params()))
    print('Tree count: {}'.format(model.tree_count_))
    print('Classes: {}'.format(model.classes_))

    class_label_to_index = dict()
    index = 0
    for class_label in model.classes_:
      class_label_to_index[class_label] = index
      index += 1

    remain_zero = len(x_train.columns) - 1
    with open(self.file_probability_path, "w") as probability_file:
      for tag in tags:
        test_x = list()
        test_x.append(tag)
        for i in range(remain_zero):
          test_x.append(0)
        label = model.predict(test_x)

        index = 0
        label_value = 0
        if is_binary_classification:
          label_value = int(label)
        else:
          label_value = label[0]

        if label_value in class_label_to_index:
          index = class_label_to_index[label_value]
        else:
          raise Exception("Logic error")

        all_probability = model.predict_proba(test_x)
        probability = all_probability[index]
        print(
          "Model prediction with x={} equal = {} "
          "with probability = {}(all probalities={})".format(
            test_x, label, probability, all_probability))

        probability_file.write(str(tag))
        for p in all_probability:
          probability_file.write(" " + str(p))
        probability_file.write("\n")

    print("learning is successfully done")
    model.save_model(self.model_file_path, format = "cbm", pool = pool)
    print("Model save to file=" + self.model_file_path)

class CommandLine:
  can_print = True
  need_continue = True
  csv_file_path = "/tmp/ml_test_file.csv"
  output_model_path = "/tmp/model.bin"
  file_probability_path = "/tmp/test_probability"
  number_tags = 2
  iterations = 300
  learning_rate = 0.1
  number_record = 10000
  feature_cat_number = 5
  feature_number_number = 5
  early_stopping_rounds = 20

  def __init__(self):
    parser = argparse.ArgumentParser(
      description = "Machine learning script")
    parser.add_argument(
      "-p", "--print", help = "-p <disable print in cout>",
      action='store_false')
    parser.add_argument(
      "-c", "--csv", help = "-c <path to csv file>",
      type = str, required = False)
    parser.add_argument(
      "-o", "--output", help = "-o <path to save model file>",
      type = str, required = False)
    parser.add_argument(
      "-r", "--result", help = "-r <results for probabilities tag>",
      type = str, required = False)
    parser.add_argument(
      "-n", "--number", help = "-n <number tags>",
      type = int, required = False)
    parser.add_argument(
      "-i", "--iter", help = "-i <iterations>",
      type = int, required = False)
    parser.add_argument(
      "-l", "--lr", help = "-l <learning rate>",
      type = float, required = False)
    argument = parser.parse_args()

    if not argument.print:
      self.can_print = argument.print
    if argument.csv:
      if os.access(os.path.dirname(argument.csv), os.W_OK):
        self.csv_file_path = argument.csv
      else:
        print("Not valid path to csv file = {}".format(argument.csv))
        self.need_continue = False
    if argument.output:
      if os.access(os.path.dirname(argument.output), os.W_OK):
        self.output_model_path = argument.output
      else:
        print("Not valid path to output model file = {}".format(argument.output))
        self.need_continue = False

    if argument.result:
      if os.access(os.path.dirname(argument.result), os.W_OK):
        self.file_probability_path = argument.result
      else:
        print("Not valid path to file = {} with probabilities of tags".format(argument.output))
        self.need_continue = False

    if argument.number:
      if argument.number > 0:
        self.number_tags = argument.number
      else:
        print("Number tags must be larger then 0")
        self.need_continue = False
    if argument.iter:
      if argument.iter > 0:
        self.iterations = argument.iter
      else:
        print("Number iterations must be larger then 0")
        self.need_continue = False
    if argument.lr:
      learning_rate = float(argument.lr)
      if (learning_rate > 0.0) and (learning_rate < 1.0):
        self.learning_rate = learning_rate
      else:
        print("learning rate must be in interval (0;1)")
        self.need_continue = False

if __name__ == "__main__":
  try:
    command_line = CommandLine()
    if command_line.need_continue:
      csv_file_path = command_line.csv_file_path
      model_file_path = command_line.output_model_path
      file_probability_path = command_line.file_probability_path
      number_tags = command_line.number_tags
      iterations = command_line.iterations
      learning_rate = command_line.learning_rate
      number_record = command_line.number_record
      feature_cat_number = command_line.feature_cat_number
      feature_number_number = command_line.feature_number_number
      early_stopping_rounds = command_line.early_stopping_rounds
      can_print = command_line.can_print

      if not can_print:
        sys.stdout = open(os.devnull, 'w')

      tag_to_label = {}
      for i in range(number_tags):
        tag_to_label[i + 1] = (i + 1) * 1000

      application = MLApplication(
        csv_file_path, model_file_path, file_probability_path,
        iterations, learning_rate, early_stopping_rounds)
      application.create_file(
        tag_to_label,
        number_record, feature_cat_number,
        feature_number_number)
      application.learn()
  except Exception as e:
    print("ML application is failed. Reason: " + str(e))