import math

import numpy


def ctr_metrics(labels, probabilities):
  labels = numpy.asarray(labels, dtype=numpy.int64)
  probabilities = numpy.clip(numpy.asarray(probabilities), 1e-7, 1.0 - 1e-7)
  logloss = -numpy.mean(
    labels * numpy.log(probabilities) + (1 - labels) * numpy.log(1.0 - probabilities))
  result = {'logloss': float(logloss)}
  if len(numpy.unique(labels)) == 2:
    result['roc_auc'] = _roc_auc(labels, probabilities)
    result['pr_auc'] = _average_precision(labels, probabilities)
  else:
    result['roc_auc'] = None
    result['pr_auc'] = None
  return result


def soft_hard_metrics(soft_activations, hard_activations):
  soft = numpy.asarray(soft_activations)
  hard = numpy.asarray(hard_activations)
  soft_binary = soft >= 0.5
  return {
    'segment_agreement_rate': float(numpy.mean(soft_binary == hard.astype(bool))),
    'mean_absolute_activation_difference': float(numpy.mean(numpy.abs(soft - hard))),
  }


def match_segment_rules(learned_rules, true_rules):
  if not learned_rules or not true_rules:
    return {
      'matches': [],
      'url_precision': 0.0,
      'url_recall': 0.0,
      'url_f1': 0.0,
      'window_accuracy': 0.0,
      'n_accuracy': 0.0,
    }
  costs = numpy.zeros((len(learned_rules), len(true_rules)), dtype=numpy.float64)
  for learned_index, learned in enumerate(learned_rules):
    for true_index, truth in enumerate(true_rules):
      learned_urls = set(learned.url_ids)
      true_urls = set(truth.url_ids)
      union = learned_urls | true_urls
      jaccard = len(learned_urls & true_urls) / len(union) if union else 1.0
      window = float(learned.window_seconds == truth.window_seconds)
      threshold = float(learned.min_visits == truth.min_visits)
      costs[learned_index, true_index] = -(jaccard + 0.25 * window + 0.25 * threshold)
  learned_indices, true_indices = _linear_sum_assignment(costs)
  matches = []
  precisions = []
  recalls = []
  window_matches = []
  threshold_matches = []
  for learned_index, true_index in zip(learned_indices, true_indices):
    learned = learned_rules[int(learned_index)]
    truth = true_rules[int(true_index)]
    intersection = len(set(learned.url_ids) & set(truth.url_ids))
    precision = intersection / len(learned.url_ids) if learned.url_ids else 0.0
    recall = intersection / len(truth.url_ids) if truth.url_ids else 0.0
    precisions.append(precision)
    recalls.append(recall)
    window_matches.append(float(learned.window_seconds == truth.window_seconds))
    threshold_matches.append(float(learned.min_visits == truth.min_visits))
    matches.append({
      'learned_segment_id': learned.segment_id,
      'true_segment_id': truth.segment_id,
      'url_precision': precision,
      'url_recall': recall,
    })
  precision = float(numpy.mean(precisions))
  recall = float(numpy.mean(recalls))
  f1 = 2.0 * precision * recall / (precision + recall) if precision + recall else 0.0
  return {
    'matches': matches,
    'url_precision': precision,
    'url_recall': recall,
    'url_f1': f1,
    'window_accuracy': float(numpy.mean(window_matches)),
    'n_accuracy': float(numpy.mean(threshold_matches)),
  }


def finite_metrics(value):
  if isinstance(value, dict):
    return all(finite_metrics(item) for item in value.values())
  if isinstance(value, list):
    return all(finite_metrics(item) for item in value)
  return value is None or not isinstance(value, float) or math.isfinite(value)


def _roc_auc(labels, probabilities):
  order = numpy.argsort(probabilities, kind='stable')
  sorted_probabilities = probabilities[order]
  ranks = numpy.empty(len(labels), dtype=numpy.float64)
  begin = 0
  while begin < len(labels):
    end = begin + 1
    while end < len(labels) and sorted_probabilities[end] == sorted_probabilities[begin]:
      end += 1
    ranks[order[begin:end]] = (begin + 1 + end) / 2.0
    begin = end
  positives = labels == 1
  positive_count = int(numpy.sum(positives))
  negative_count = len(labels) - positive_count
  positive_rank_sum = float(numpy.sum(ranks[positives]))
  score = positive_rank_sum - positive_count * (positive_count + 1) / 2.0
  return score / (positive_count * negative_count)


def _average_precision(labels, probabilities):
  order = numpy.argsort(-probabilities, kind='stable')
  sorted_labels = labels[order]
  cumulative_positives = numpy.cumsum(sorted_labels)
  ranks = numpy.arange(1, len(labels) + 1)
  positive_count = int(cumulative_positives[-1])
  values = cumulative_positives[sorted_labels == 1] / ranks[sorted_labels == 1]
  return float(numpy.sum(values) / positive_count)


def _linear_sum_assignment(costs):
  costs = numpy.asarray(costs, dtype=numpy.float64)
  transposed = costs.shape[0] > costs.shape[1]
  if transposed:
    costs = costs.T
  rows, columns = costs.shape
  row_potential = numpy.zeros(rows + 1)
  column_potential = numpy.zeros(columns + 1)
  matching = numpy.zeros(columns + 1, dtype=numpy.int64)
  path = numpy.zeros(columns + 1, dtype=numpy.int64)
  for row in range(1, rows + 1):
    matching[0] = row
    min_values = numpy.full(columns + 1, numpy.inf)
    used = numpy.zeros(columns + 1, dtype=bool)
    column = 0
    while True:
      used[column] = True
      current_row = matching[column]
      delta = numpy.inf
      next_column = 0
      for candidate_column in range(1, columns + 1):
        if used[candidate_column]:
          continue
        value = (
          costs[current_row - 1, candidate_column - 1] -
          row_potential[current_row] - column_potential[candidate_column])
        if value < min_values[candidate_column]:
          min_values[candidate_column] = value
          path[candidate_column] = column
        if min_values[candidate_column] < delta:
          delta = min_values[candidate_column]
          next_column = candidate_column
      for candidate_column in range(columns + 1):
        if used[candidate_column]:
          row_potential[matching[candidate_column]] += delta
          column_potential[candidate_column] -= delta
        else:
          min_values[candidate_column] -= delta
      column = next_column
      if matching[column] == 0:
        break
    while True:
      previous_column = path[column]
      matching[column] = matching[previous_column]
      column = previous_column
      if column == 0:
        break
  row_indices = []
  column_indices = []
  for column in range(1, columns + 1):
    if matching[column]:
      row_indices.append(int(matching[column] - 1))
      column_indices.append(column - 1)
  if transposed:
    return numpy.asarray(column_indices), numpy.asarray(row_indices)
  return numpy.asarray(row_indices), numpy.asarray(column_indices)
