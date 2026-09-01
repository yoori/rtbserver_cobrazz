import copy
import dataclasses
import json
import pathlib

import numpy
import torch

from .HardSegmentLayer import HardSegmentLayer
from .SegmentCTRModel import SegmentCTRModel
from .SegmentModelData import BatchRequest
from .SegmentModelData import ForkedBatchPool
from .SegmentModelLoss import segment_model_loss
from .SegmentModelMetrics import ctr_metrics
from .SegmentModelMetrics import match_segment_rules
from .SegmentModelMetrics import soft_hard_metrics
from .SegmentRuleExtractor import extract_segment_rules
from .SyntheticSegmentData import SyntheticBatchBuilder
from .TemperatureScheduler import TemperatureScheduler
from .TrainingProgress import TrainingProgressReporter


class SegmentModelTrainer:
  def __init__(self, config, dataset, training_builder=None, validation_builder=None,
               final_test_builder=None):
    self.config = config
    self.dataset = dataset
    self.training_builder = training_builder
    self.validation_builder = validation_builder
    self.final_test_builder = final_test_builder
    self.training_summary = None
    self.result_temperature_progress = 1.0
    torch.manual_seed(config.training.seed)
    numpy.random.seed(config.training.seed)
    self.device = torch.device(config.training.device)
    self.model = SegmentCTRModel(
      config,
      config.data.url_buckets,
      dataset.existing_channels.shape[1]).to(self.device)
    self._initialize_membership()
    self.optimizer = torch.optim.AdamW(
      self.model.parameters(),
      lr=config.training.learning_rate,
      weight_decay=config.training.weight_decay)
    self.temperature_schedulers = {
      name: TemperatureScheduler(getattr(config, name + '_temperature'))
      for name in (
        'url',
        'window',
        'threshold',
        'activation',
        'aggregation',
        'forest_feature',
        'forest_split',
      )
    }

  def train(self, checkpoint_dir=None):
    if checkpoint_dir is not None:
      checkpoint_dir = pathlib.Path(checkpoint_dir)
      checkpoint_dir.mkdir(parents=True, exist_ok=True)
      _write_json(checkpoint_dir / 'config.json', dataclasses.asdict(self.config))
    builder = self.training_builder
    if builder is None:
      builder = SyntheticBatchBuilder(
        self.dataset,
        self.dataset.train_indices,
        self.config.data.batch_size,
        self.config.training.seed)
    validation_builder = self.validation_builder
    if validation_builder is None:
      validation_builder = SyntheticBatchBuilder(
        self.dataset,
        self.dataset.validation_indices,
        self.config.data.batch_size,
        self.config.training.seed + 100000)
    structuring_batches = self.config.training.structuring_epochs * builder.batches_per_epoch
    total_batches = self.config.training.max_epochs * builder.batches_per_epoch
    completed_batches = 0
    history = []
    best_model_state = None
    best_optimizer_state = None
    best_validation_loss = float('inf')
    best_epoch = None
    best_temperature_progress = None
    epochs_without_improvement = 0
    stopped_early = False
    progress_reporter = TrainingProgressReporter(total_batches)
    progress_reporter.start()
    try:
      for epoch in range(self.config.training.max_epochs):
        include_existing = epoch >= self.config.training.discovery_epochs
        stage = 'structuring' if include_existing else 'discovery'
        requests = (
          BatchRequest(epoch, batch_index, include_existing)
          for batch_index in range(builder.batches_per_epoch)
        )
        epoch_values = []
        with ForkedBatchPool(
            builder,
            requests,
            self.config.data.batch_workers,
            self.config.data.ready_batches,
            self.config.data.batch_start_method,
            progress_reporter.maybe_report) as batches:
          batch_iterator = iter(batches)
          for batch_index in range(builder.batches_per_epoch):
            progress_reporter.set_position(stage, epoch)
            progress_reporter.begin_batch_wait()
            try:
              batch = next(batch_iterator)
            finally:
              progress_reporter.end_batch_wait()
            training_progress = _structuring_progress(
              epoch,
              batch_index,
              builder.batches_per_epoch,
              self.config.training.discovery_epochs,
              structuring_batches)
            regularization_scale = training_progress if include_existing else 0.0
            progress_reporter.begin_training_compute()
            try:
              result = self._train_batch(
                batch,
                self.temperatures(training_progress),
                regularization_scale)
            except BaseException:
              progress_reporter.end_training_compute(completed=False)
              raise
            else:
              progress_reporter.end_training_compute()
            progress_reporter.maybe_report()
            epoch_values.append(result)
            completed_batches += 1
        validation_loss = self._validation_loss(
          validation_builder,
          epoch,
          include_existing,
          self.temperatures(training_progress),
          progress_reporter.maybe_report)
        improved = (
          validation_loss <
          best_validation_loss - self.config.training.early_stopping_min_delta)
        if improved:
          best_validation_loss = validation_loss
          best_epoch = epoch
          best_temperature_progress = training_progress
          best_model_state = _state_to_cpu(self.model.state_dict())
          best_optimizer_state = _state_to_cpu(self.optimizer.state_dict())
          epochs_without_improvement = 0
        else:
          epochs_without_improvement += 1
        self._record_epoch(
          history,
          stage,
          epoch,
          epoch_values,
          validation_loss,
          best_validation_loss,
          epochs_without_improvement,
          improved)
        if checkpoint_dir is not None and epoch + 1 == self.config.training.discovery_epochs:
          self._save_checkpoint(checkpoint_dir / 'checkpoint-discovery.pt', 'discovery')
        if epochs_without_improvement >= self.config.training.early_stopping_patience:
          stopped_early = True
          break
    finally:
      progress_reporter.close()
    if best_model_state is None:
      raise RuntimeError('training did not produce a validation checkpoint')
    self.model.load_state_dict(best_model_state)
    self.optimizer.load_state_dict(best_optimizer_state)
    self.result_temperature_progress = best_temperature_progress
    self.training_summary = {
      'max_epochs': self.config.training.max_epochs,
      'epochs_completed': len(history),
      'stopped_early': stopped_early,
      'patience': self.config.training.early_stopping_patience,
      'min_delta': self.config.training.early_stopping_min_delta,
      'best_epoch': best_epoch,
      'best_validation_loss': best_validation_loss,
      'best_temperatures': self.temperatures(best_temperature_progress),
    }
    if checkpoint_dir is not None:
      self._save_checkpoint(checkpoint_dir / 'checkpoint-best.pt', 'best-validation')
    return history

  def evaluate(self):
    self.model.eval()
    rules = extract_segment_rules(
      self.model,
      self.dataset.urls,
      self.dataset.url_bucket_ids)
    hard_segment_layer = HardSegmentLayer(
      rules,
      self.config.data.url_buckets,
      self.config.data.windows_seconds,
      self.config.model.aggregation).to(self.device)
    builder = self.final_test_builder
    if builder is None:
      builder = SyntheticBatchBuilder(
        self.dataset,
        self.dataset.final_test_indices,
        self.config.data.batch_size,
        self.config.training.seed + 200000)
    requests = (
      BatchRequest(0, batch_index, True)
      for batch_index in range(builder.batches_per_epoch)
    )
    temperatures = self.temperatures(self.result_temperature_progress)
    labels = []
    soft_probabilities = []
    hard_probabilities = []
    soft_activations = []
    hard_activations = []
    existing_channels = []
    sample_indices = []
    group_ids = []
    variant_ids = []
    with torch.no_grad(), ForkedBatchPool(
        builder,
        requests,
        self.config.data.batch_workers,
        self.config.data.ready_batches,
        self.config.data.batch_start_method) as batches:
      for batch in batches:
        tensors = self._batch_tensors(batch)
        output = self.model(*tensors[:3], temperatures, tensors[4])
        hard = hard_segment_layer(tensors[0], tensors[4])
        hard_logits = self.model.segment_logits(hard, tensors[2], temperatures)
        labels.append(tensors[3].cpu().numpy())
        soft_probabilities.append(torch.sigmoid(output.logits).cpu().numpy())
        hard_probabilities.append(torch.sigmoid(hard_logits).cpu().numpy())
        soft_activations.append(output.segment_output.activations.cpu().numpy())
        hard_activations.append(hard.cpu().numpy())
        existing_channels.append(tensors[1].cpu().numpy())
        sample_indices.append(batch.sample_indices)
        if batch.group_ids is not None:
          group_ids.append(batch.group_ids)
        if batch.variant_ids is not None:
          variant_ids.append(batch.variant_ids)
    labels = numpy.concatenate(labels)
    soft_probabilities = numpy.concatenate(soft_probabilities)
    hard_probabilities = numpy.concatenate(hard_probabilities)
    soft_activations = numpy.concatenate(soft_activations)
    hard_activations = numpy.concatenate(hard_activations)
    existing_channels = numpy.concatenate(existing_channels)
    sample_indices = numpy.concatenate(sample_indices)
    existing_channel_ids = [rule.channel_id for rule in self.dataset.existing_rules]
    rules = extract_segment_rules(
      self.model,
      self.dataset.urls,
      self.dataset.url_bucket_ids,
      hard_activations,
      existing_channels,
      existing_channel_ids)
    metrics = {
      'soft_ctr': ctr_metrics(labels, soft_probabilities),
      'hard_ctr': ctr_metrics(labels, hard_probabilities),
      'soft_hard': soft_hard_metrics(soft_activations, hard_activations),
      'recovery': match_segment_rules(rules, self.dataset.true_rules),
      'diagnostics': self._diagnostics(rules, soft_activations),
    }
    if group_ids or self.dataset.group_ids is not None:
      evaluated_group_ids = (
        numpy.concatenate(group_ids)
        if group_ids else self.dataset.group_ids[sample_indices])
      evaluated_variant_ids = None
      if variant_ids:
        evaluated_variant_ids = numpy.concatenate(variant_ids)
      elif self.dataset.variant_ids is not None:
        evaluated_variant_ids = self.dataset.variant_ids[sample_indices]
      metrics['groups'] = _group_metrics(
        labels,
        soft_probabilities,
        hard_probabilities,
        evaluated_group_ids,
        self.dataset.group_names,
        evaluated_variant_ids,
        self.dataset.group_variant_names)
    return metrics, rules

  def temperatures(self, progress):
    return {
      name: scheduler.value(progress)
      for name, scheduler in self.temperature_schedulers.items()
    }

  def _train_batch(self, batch, temperatures, regularization_scale=1.0):
    self.model.train()
    tensors = self._batch_tensors(batch)
    self.optimizer.zero_grad(set_to_none=True)
    output = self.model(*tensors[:3], temperatures, tensors[4])
    loss = segment_model_loss(
      output,
      tensors[3],
      self.config,
      regularization_scale)
    loss.total.backward()
    self.optimizer.step()
    return {
      **{
        field.name: float(getattr(loss, field.name).detach().cpu())
        for field in dataclasses.fields(loss)
      },
      'average_segment_activation': output.segment_output.activations.detach().mean(
        dim=0).cpu().tolist(),
      'regularization_scale': regularization_scale,
      'temperatures': temperatures,
    }

  def _batch_tensors(self, batch):
    return (
      torch.from_numpy(batch.history_counts).to(self.device),
      torch.from_numpy(batch.existing_channels).to(self.device),
      torch.from_numpy(batch.context_features).to(self.device),
      torch.from_numpy(batch.labels).to(self.device),
      torch.from_numpy(batch.history_url_ids).to(self.device),
    )

  def _initialize_membership(self):
    membership_config = self.config.model.membership
    observed_buckets = numpy.asarray(self.dataset.history_url_ids, dtype=numpy.int64)
    if not len(observed_buckets):
      raise ValueError('URL bucket dictionary must not be empty')
    if membership_config.initialization == 'frequency' and self.dataset.history_counts is None:
      raise ValueError('frequency membership initialization requires materialized training data')
    if membership_config.initialization == 'frequency':
      frequencies = numpy.sum(
        self.dataset.history_counts[self.dataset.train_indices, :, -1],
        axis=0)
      url_ranking = observed_buckets[numpy.argsort(-frequencies, kind='stable')]
    else:
      generator = numpy.random.default_rng(self.config.training.seed)
      url_ranking = generator.permutation(observed_buckets)
    selected_count = self.config.model.candidates * membership_config.initial_urls_per_candidate
    selected_urls = numpy.resize(url_ranking, selected_count).reshape(
      self.config.model.candidates,
      membership_config.initial_urls_per_candidate)
    url_logits = self.model.segment_layer.membership.url_logits
    with torch.no_grad():
      url_logits.normal_(membership_config.unselected_logit, membership_config.logit_std)
      for candidate, url_ids in enumerate(selected_urls):
        values = torch.empty(len(url_ids), dtype=url_logits.dtype, device=url_logits.device)
        values.normal_(membership_config.selected_logit, membership_config.logit_std)
        indices = torch.as_tensor(url_ids, dtype=torch.long, device=url_logits.device)
        url_logits[candidate, indices] = values

  def save(self, output_dir, history, metrics, rules):
    output_dir = pathlib.Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    self._save_checkpoint(output_dir / 'checkpoint.pt', 'complete')
    _write_json(output_dir / 'config.json', dataclasses.asdict(self.config))
    _write_json(output_dir / 'training.json', history)
    _write_json(output_dir / 'metrics.json', metrics)
    _write_json(output_dir / 'segments.json', [rule.to_dict() for rule in rules])
    _write_json(output_dir / 'synthetic-ground-truth.json', self.ground_truth())
    _write_json(output_dir / 'vocabulary.json', {
      'urls': self.dataset.urls,
      'url_buckets': self.config.data.url_buckets,
      'existing_channel_ids': [rule.channel_id for rule in self.dataset.existing_rules],
    })
    _write_json(output_dir / 'url-bucket-dictionary.json', {
      str(bucket): urls
      for bucket, urls in sorted(self.dataset.url_bucket_dictionary.items())
    })
    if self.dataset.source_metadata is not None:
      _write_json(output_dir / 'source.json', self.dataset.source_metadata)
    if self.training_summary is not None:
      _write_json(output_dir / 'training-summary.json', self.training_summary)

  def ground_truth(self):
    return [
      {
        'segment_id': rule.segment_id,
        'urls': [self.dataset.urls[url_id] for url_id in rule.url_ids],
        'window_seconds': rule.window_seconds,
        'min_visits': rule.min_visits,
        'weight': rule.weight,
      }
      for rule in self.dataset.true_rules
    ]

  def _save_checkpoint(self, checkpoint_file, stage):
    temporary_file = checkpoint_file.with_name(checkpoint_file.name + '.tmp')
    torch.save({
      'model': self.model.state_dict(),
      'optimizer': self.optimizer.state_dict(),
      'stage': stage,
    }, temporary_file)
    temporary_file.replace(checkpoint_file)

  def _record_epoch(self, history, stage, epoch, values, validation_loss,
                    best_validation_loss, epochs_without_improvement, improved):
    record = _epoch_record(
      stage,
      epoch,
      values,
      validation_loss,
      best_validation_loss,
      epochs_without_improvement,
      improved)
    history.append(record)
    print(json.dumps(record, sort_keys=True), flush=True)

  def _validation_loss(
      self,
      builder,
      epoch,
      include_existing_channels,
      temperatures,
      wait_callback=None):
    self.model.eval()
    requests = (
      BatchRequest(epoch, batch_index, include_existing_channels)
      for batch_index in range(builder.batches_per_epoch)
    )
    total_loss = 0.0
    rows = 0
    with torch.no_grad(), ForkedBatchPool(
        builder,
        requests,
        self.config.data.batch_workers,
        self.config.data.ready_batches,
        self.config.data.batch_start_method,
        wait_callback) as batches:
      for batch in batches:
        tensors = self._batch_tensors(batch)
        output = self.model(*tensors[:3], temperatures, tensors[4])
        total_loss += float(torch.nn.functional.binary_cross_entropy_with_logits(
          output.logits,
          tensors[3],
          reduction='sum').cpu())
        rows += len(batch.labels)
    if not rows:
      raise RuntimeError('validation dataset is empty')
    return total_loss / rows

  def _diagnostics(self, rules, soft_activations):
    temperatures = self.temperatures(self.result_temperature_progress)
    with torch.no_grad():
      url_gates = self.model.segment_layer.membership.gates(temperatures['url']).cpu().numpy()
      window_gates = torch.softmax(
        self.model.segment_layer.window_logits / temperatures['window'],
        dim=1).cpu().numpy()
      threshold_gates = torch.softmax(
        self.model.segment_layer.threshold_logits / temperatures['threshold'],
        dim=1).cpu().numpy()
    hard_sets = [set(rule.url_ids) for rule in rules]
    highly_similar = 0
    for first in range(len(hard_sets)):
      for second in range(first + 1, len(hard_sets)):
        union = hard_sets[first] | hard_sets[second]
        similarity = len(hard_sets[first] & hard_sets[second]) / len(union) if union else 1.0
        highly_similar += int(similarity >= 0.9)
    candidate_diagnostics = []
    observed_gates = url_gates[:, self.dataset.url_bucket_ids]
    regularized_gates = url_gates[:, self.dataset.history_url_ids]
    for candidate, rule in enumerate(rules):
      top_url_ids = numpy.argsort(-observed_gates[candidate])[:10]
      candidate_diagnostics.append({
        'segment_id': candidate,
        'top_url_gates': [
          {
            'url': self.dataset.urls[int(url_id)],
            'bucket_id': int(self.dataset.url_bucket_ids[url_id]),
            'gate': float(observed_gates[candidate, url_id]),
          }
          for url_id in top_url_ids
        ],
        'window_probabilities': {
          str(window): float(probability)
          for window, probability in zip(self.config.data.windows_seconds, window_gates[candidate])
        },
        'n_probabilities': {
          str(value): float(probability)
          for value, probability in zip(self.config.data.n_values, threshold_gates[candidate])
        },
        'average_activation': float(numpy.mean(soft_activations[:, candidate])),
        'forest_split_count': rule.forest_split_count,
      })
    extracted_sizes = [len(rule.url_ids) for rule in rules]
    return {
      'temperatures': temperatures,
      'average_urls_per_extracted_segment': float(numpy.mean(extracted_sizes)),
      'empty_segments': int(sum(size == 0 for size in extracted_sizes)),
      'highly_similar_segment_pairs': highly_similar,
      'url_gate_fraction_near_zero': float(numpy.mean(regularized_gates <= 0.1)),
      'url_gate_fraction_near_one': float(numpy.mean(regularized_gates >= 0.9)),
      'url_gate_fraction_ambiguous': float(numpy.mean(
        (regularized_gates > 0.1) & (regularized_gates < 0.9))),
      'candidates': candidate_diagnostics,
    }


def _structuring_progress(epoch, batch_index, batches_per_epoch, discovery_epochs,
                          structuring_batches):
  if epoch < discovery_epochs:
    return 0.0
  if structuring_batches <= 1:
    return 1.0
  completed = (epoch - discovery_epochs) * batches_per_epoch + batch_index
  return min(completed / (structuring_batches - 1), 1.0)


def _epoch_record(stage, epoch, values, validation_loss, best_validation_loss,
                  epochs_without_improvement, improved):
  return {
    'stage': stage,
    'epoch': epoch,
    'validation_loss': validation_loss,
    'best_validation_loss': best_validation_loss,
    'epochs_without_improvement': epochs_without_improvement,
    'best': improved,
    **{
      name: float(numpy.mean([value[name] for value in values]))
      for name in values[0]
      if name not in ('average_segment_activation', 'temperatures')
    },
    'average_segment_activation': numpy.mean(
      [value['average_segment_activation'] for value in values],
      axis=0).tolist(),
    'temperatures': values[-1]['temperatures'],
  }


def _state_to_cpu(value):
  if isinstance(value, torch.Tensor):
    return value.detach().cpu().clone()
  if isinstance(value, dict):
    return {key: _state_to_cpu(item) for key, item in value.items()}
  if isinstance(value, list):
    return [_state_to_cpu(item) for item in value]
  if isinstance(value, tuple):
    return tuple(_state_to_cpu(item) for item in value)
  return copy.deepcopy(value)


def _write_json(output_file, value):
  temporary_file = output_file.with_name(output_file.name + '.tmp')
  temporary_file.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n', encoding='utf-8')
  temporary_file.replace(output_file)


def _group_metrics(
  labels,
  soft_probabilities,
  hard_probabilities,
  group_ids,
  group_names,
  variant_ids=None,
  group_variant_names=()):
  result = {}
  for group_id, group_name in enumerate(group_names):
    selected = group_ids == group_id
    group_result = _prediction_metrics(
      labels[selected],
      soft_probabilities[selected],
      hard_probabilities[selected])
    if variant_ids is not None:
      group_result['variants'] = {}
      for variant_id, variant_name in enumerate(group_variant_names[group_id]):
        variant_selected = selected & (variant_ids == variant_id)
        group_result['variants'][variant_name] = _prediction_metrics(
          labels[variant_selected],
          soft_probabilities[variant_selected],
          hard_probabilities[variant_selected])
    result[group_name] = group_result
  return result


def _prediction_metrics(labels, soft_probabilities, hard_probabilities):
  if not len(labels):
    return {
      'rows': 0,
      'clicks': 0,
      'actual_ctr': None,
      'average_soft_ctr': None,
      'average_hard_ctr': None,
      'soft_ctr': None,
      'hard_ctr': None,
    }
  return {
    'rows': len(labels),
    'clicks': int(numpy.sum(labels)),
    'actual_ctr': float(numpy.mean(labels)),
    'average_soft_ctr': float(numpy.mean(soft_probabilities)),
    'average_hard_ctr': float(numpy.mean(hard_probabilities)),
    'soft_ctr': ctr_metrics(labels, soft_probabilities),
    'hard_ctr': ctr_metrics(labels, hard_probabilities),
  }
