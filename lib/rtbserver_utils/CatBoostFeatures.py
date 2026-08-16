import numpy as np
import scipy.sparse
from sklearn.datasets import load_svmlight_file


def fold_features(data, features_size):
  if features_size <= 0:
    raise ValueError('features_size must be positive')

  if isinstance(data, np.ndarray):
    result = np.zeros((data.shape[0], features_size), dtype=data.dtype)
    rows, columns = data.nonzero()
    for row, column in zip(rows, columns):
      result[row, column % features_size] = data[row, column]
    return result

  if scipy.sparse.issparse(data):
    data = data.tocsr()
    result = scipy.sparse.lil_matrix(
      (data.shape[0], features_size),
      dtype=data.dtype)
    for row in range(data.shape[0]):
      for position in range(data.indptr[row], data.indptr[row + 1]):
        column = data.indices[position]
        result[row, column % features_size] = data.data[position]
    return result.tocsr()

  raise TypeError("Unsupported features type: " + str(type(data)))


def load_catboost_svm(svm_file, features_size):
  data, labels = load_svmlight_file(svm_file)
  return fold_features(data, features_size), labels
