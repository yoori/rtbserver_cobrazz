import copy
import dataclasses
import json
import math
import pathlib

import numpy
import torch

from .CandidateDuplicate import activation_similarity_matrix
from .CandidateDuplicate import soft_jaccard_matrix
from .CandidateOpeningScheduler import CandidateOpeningScheduler
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


@dataclasses.dataclass
class ValidationResult:
  loss: float
  activation_sum: torch.Tensor
  activation_cross_product: torch.Tensor
  rows: int


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
    self.result_temperatures = None
    self.reseeded_candidates = set()
    self.candidate_opening_scheduler = CandidateOpeningScheduler(
      config.candidate_opening,
      config.model.candidates)
    self.current_candidate_opening_state = self.candidate_opening_scheduler.state(0)
    self.result_candidate_opening_state = self.current_candidate_opening_state
    self.candidate_opening_events = []
    torch.manual_seed(config.training.seed)
    numpy.random.seed(config.training.seed)
    self.device = torch.device(config.training.device)
    self.model = SegmentCTRModel(
      config,
      config.data.url_buckets,
      dataset.existing_channels.shape[1]).to(self.device)
    self._initialize_membership()
    self.training_rows, self.training_clicks = self._training_label_statistics()
    self.training_ctr = self.training_clicks / self.training_rows
    self.initial_global_bias = self._initialize_global_bias(self.training_ctr)
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

  def train(self, checkpoint_dir=None, progress_callback=None):
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
    best_temperatures = None
    best_opening_state = None
    epochs_without_improvement = 0
    stopped_early = False
    progress_reporter = TrainingProgressReporter(
      total_batches,
      callback=progress_callback)
    progress_reporter.start()
    try:
      for epoch in range(self.config.training.max_epochs):
        opening_state = self._apply_candidate_opening(epoch)
        include_existing = epoch >= self.config.training.discovery_epochs
        stage = self._training_stage(include_existing, opening_state)
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
            opening_state = self.candidate_opening_scheduler.state(
              epoch + batch_index / builder.batches_per_epoch)
            regularization_scale = training_progress if include_existing else 0.0
            duplicate_regularization_scale = _duplicate_regularization_scale(
              epoch,
              batch_index,
              builder.batches_per_epoch,
              self.config.loss.duplicate_regularization_start_epoch,
              self.config.loss.duplicate_regularization_ramp_epochs)
            progress_reporter.begin_training_compute()
            try:
              result = self._train_batch(
                batch,
                self.temperatures(training_progress, opening_state),
                opening_state,
                regularization_scale,
                duplicate_regularization_scale)
            except BaseException:
              progress_reporter.end_training_compute(completed=False)
              raise
            else:
              progress_reporter.end_training_compute()
            progress_reporter.maybe_report()
            epoch_values.append(result)
            completed_batches += 1
        temperatures = self.temperatures(training_progress, opening_state)
        validation = self._validation_loss(
          validation_builder,
          epoch,
          include_existing,
          temperatures,
          progress_reporter.maybe_report)
        validation_loss = validation.loss
        duplicate_diagnostics, duplicate_pairs = self._candidate_duplicate_diagnostics(
          temperatures,
          validation.activation_sum,
          validation.activation_cross_product,
          validation.rows)
        opening_diagnostics = self._candidate_opening_diagnostics(
          temperatures,
          opening_state)
        duplicate_schedule_complete = duplicate_regularization_scale >= 1.0
        reseeded_candidates = []
        if self.config.loss.enable_candidate_reseed and duplicate_schedule_complete:
          reseeded_candidates = self._reseed_duplicate_candidates(
            duplicate_pairs,
            duplicate_diagnostics['forest_importance'],
            epoch)
        checkpoint_eligible = (
          opening_state.all_candidates_open and
          (duplicate_schedule_complete or not _duplicate_regularization_enabled(self.config)) and
          not reseeded_candidates and
          (not self.config.loss.enable_candidate_reseed or not duplicate_pairs))
        improved = False
        if checkpoint_eligible:
          improved = (
            validation_loss <
            best_validation_loss - self.config.training.early_stopping_min_delta)
          if improved:
            best_validation_loss = validation_loss
            best_epoch = epoch
            best_temperature_progress = training_progress
            best_temperatures = dict(temperatures)
            best_opening_state = opening_state
            best_model_state = _state_to_cpu(self.model.state_dict())
            best_optimizer_state = _state_to_cpu(self.optimizer.state_dict())
            epochs_without_improvement = 0
          elif opening_state.joint_finetune_complete:
            epochs_without_improvement += 1
          else:
            epochs_without_improvement = 0
        else:
          epochs_without_improvement = 0
        self._record_epoch(
          history,
          stage,
          epoch,
          epoch_values,
          validation_loss,
          best_validation_loss,
          epochs_without_improvement,
          improved,
          duplicate_diagnostics,
          opening_diagnostics,
          reseeded_candidates,
          checkpoint_eligible)
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
    self.result_temperatures = best_temperatures
    self.result_candidate_opening_state = best_opening_state
    self.training_summary = {
      'max_epochs': self.config.training.max_epochs,
      'epochs_completed': len(history),
      'stopped_early': stopped_early,
      'patience': self.config.training.early_stopping_patience,
      'min_delta': self.config.training.early_stopping_min_delta,
      'best_epoch': best_epoch,
      'best_validation_loss': best_validation_loss,
      'best_temperatures': best_temperatures,
      'candidate_opening_events': self.candidate_opening_events,
      'best_candidate_mask': [int(value) for value in best_opening_state.candidate_mask],
      'training_rows': self.training_rows,
      'training_clicks': self.training_clicks,
      'training_ctr': self.training_ctr,
      'initial_global_bias': self.initial_global_bias,
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
    temperatures = self.result_temperatures or self.temperatures(
      self.result_temperature_progress,
      self.result_candidate_opening_state)
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
        hard_logits = self.model.segment_logits(hard, tensors[1], tensors[2], temperatures)
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

  def temperatures(self, progress, opening_state=None):
    temperatures = {
      name: scheduler.value(progress)
      for name, scheduler in self.temperature_schedulers.items()
    }
    if opening_state is not None:
      temperatures['url'] = self.candidate_opening_scheduler.url_temperature(
        temperatures['url'],
        opening_state.position,
        self.config.url_temperature.schedule)
    return temperatures

  def _train_batch(
      self,
      batch,
      temperatures,
      opening_state,
      regularization_scale=1.0,
      duplicate_regularization_scale=1.0):
    self.model.train()
    tensors = self._batch_tensors(batch)
    self.optimizer.zero_grad(set_to_none=True)
    output = self.model(*tensors[:3], temperatures, tensors[4])
    loss = segment_model_loss(
      output,
      tensors[3],
      self.config,
      regularization_scale,
      duplicate_regularization_scale)
    loss.total.backward()
    self._mask_closed_candidate_gradients(opening_state)
    snapshots = self._candidate_parameter_snapshots(opening_state)
    self.optimizer.step()
    self._scale_candidate_parameter_updates(snapshots)
    return {
      **{
        field.name: float(getattr(loss, field.name).detach().cpu())
        for field in dataclasses.fields(loss)
      },
      'average_segment_activation': output.segment_output.activations.detach().mean(
        dim=0).cpu().tolist(),
      'regularization_scale': regularization_scale,
      'duplicate_regularization_scale': duplicate_regularization_scale,
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

  def _apply_candidate_opening(self, epoch):
    state = self.candidate_opening_scheduler.state(epoch)
    previous_mask = tuple(
      bool(value)
      for value in self.model.candidate_mask_layer.candidate_mask.detach().cpu().tolist())
    if previous_mask != state.candidate_mask:
      self.model.set_candidate_mask(state.candidate_mask)
      opened_candidates = [
        candidate
        for candidate, (previous, current) in enumerate(
          zip(previous_mask, state.candidate_mask))
        if not previous and current
      ]
      for candidate in opened_candidates:
        for parameter in self._candidate_parameters():
          self._reset_optimizer_candidate(parameter, candidate)
        event = {
          'event': 'candidate_open',
          'candidate': candidate,
          'epoch': epoch,
          'mask_before': [int(value) for value in previous_mask],
          'mask_after': [int(value) for value in state.candidate_mask],
        }
        self.candidate_opening_events.append(event)
        print(json.dumps(event, sort_keys=True), flush=True)
    self.current_candidate_opening_state = state
    return state

  def _training_stage(self, include_existing, opening_state):
    if not include_existing:
      return 'discovery'
    if self.config.candidate_opening.enabled and not opening_state.all_candidates_open:
      return 'candidate_opening'
    if opening_state.joint_finetune_active:
      return 'joint_finetune'
    return 'structuring'

  def _candidate_parameters(self):
    return tuple(
      parameter
      for parameter in (
        self.model.segment_layer.membership.url_logits,
        self.model.segment_layer.window_logits,
        self.model.segment_layer.threshold_logits,
      )
      if parameter is not None)

  def _mask_closed_candidate_gradients(self, opening_state):
    active = torch.as_tensor(
      opening_state.candidate_mask,
      dtype=torch.bool,
      device=self.device)
    for parameter in self._candidate_parameters():
      if parameter.grad is None:
        continue
      shape = (len(active),) + (1,) * (parameter.grad.ndim - 1)
      parameter.grad.mul_(active.reshape(shape).to(parameter.grad.dtype))

  def _candidate_parameter_snapshots(self, opening_state):
    multipliers = torch.as_tensor(
      opening_state.learning_rate_multipliers,
      dtype=torch.float32,
      device=self.device)
    indices = torch.nonzero(multipliers != 1.0, as_tuple=False).flatten()
    if not len(indices):
      return []
    return [
      (
        parameter,
        indices,
        parameter.detach().index_select(0, indices).clone(),
        multipliers.index_select(0, indices).to(parameter.dtype),
      )
      for parameter in self._candidate_parameters()
    ]

  @staticmethod
  def _scale_candidate_parameter_updates(snapshots):
    with torch.no_grad():
      for parameter, indices, before, multipliers in snapshots:
        updated = parameter.index_select(0, indices)
        shape = (len(multipliers),) + (1,) * (updated.ndim - 1)
        scaled = before + (updated - before) * multipliers.reshape(shape)
        parameter.index_copy_(0, indices, scaled)

  def _initialize_membership(self):
    membership_config = self.config.model.membership
    observed_buckets = numpy.asarray(self.dataset.history_url_ids, dtype=numpy.int64)
    if not len(observed_buckets):
      raise ValueError('URL bucket dictionary must not be empty')
    if membership_config.initialization == 'frequency' and self.dataset.history_counts is None:
      raise ValueError('frequency membership initialization requires materialized training data')
    url_logits = self.model.segment_layer.membership.url_logits
    with torch.no_grad():
      url_logits.normal_(membership_config.unselected_logit, membership_config.logit_std)
    if membership_config.initialization == 'symmetric_with_noise':
      return
    if membership_config.initialization == 'frequency':
      frequencies = numpy.sum(
        self.dataset.history_counts[self.dataset.train_indices, :, -1],
        axis=0)
      url_ranking = observed_buckets[numpy.argsort(-frequencies, kind='stable')]
    else:
      generator = numpy.random.default_rng(self.config.training.seed)
      url_ranking = generator.permutation(observed_buckets)
    urls_per_candidate = membership_config.initial_urls_per_candidate
    if membership_config.initialization == 'random_single_seed':
      urls_per_candidate = 1
    if membership_config.initialization == 'random_multi_seed':
      replace = urls_per_candidate > len(observed_buckets)
      selected_urls = numpy.stack([
        generator.choice(observed_buckets, urls_per_candidate, replace=replace)
        for _ in range(self.config.model.candidates)
      ])
    else:
      selected_count = self.config.model.candidates * urls_per_candidate
      selected_urls = numpy.resize(url_ranking, selected_count).reshape(
        self.config.model.candidates,
        urls_per_candidate)
    with torch.no_grad():
      for candidate, url_ids in enumerate(selected_urls):
        values = torch.empty(len(url_ids), dtype=url_logits.dtype, device=url_logits.device)
        values.normal_(membership_config.selected_logit, membership_config.logit_std)
        indices = torch.as_tensor(url_ids, dtype=torch.long, device=url_logits.device)
        url_logits[candidate, indices] = values

  def _training_label_statistics(self):
    if self.training_builder is not None:
      if not hasattr(self.training_builder, 'label_statistics'):
        raise ValueError('training batch builder must provide label_statistics')
      rows, clicks = self.training_builder.label_statistics()
    else:
      labels = self.dataset.labels[self.dataset.train_indices]
      rows = len(labels)
      clicks = float(numpy.sum(labels, dtype=numpy.float64))
    if rows <= 0 or not 0 <= clicks <= rows:
      raise ValueError('training label statistics are invalid')
    return int(rows), float(clicks)

  def _initialize_global_bias(self, ctr):
    epsilon = torch.finfo(self.model.forest.global_bias.dtype).eps
    clipped_ctr = min(1.0 - epsilon, max(epsilon, ctr))
    bias = math.log(clipped_ctr / (1.0 - clipped_ctr))
    with torch.no_grad():
      self.model.forest.global_bias.fill_(bias)
    return bias

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
                    best_validation_loss, epochs_without_improvement, improved,
                    duplicate_diagnostics, opening_diagnostics, reseeded_candidates,
                    checkpoint_eligible):
    record = _epoch_record(
      stage,
      epoch,
      values,
      validation_loss,
      best_validation_loss if math.isfinite(best_validation_loss) else None,
      epochs_without_improvement,
      improved)
    record['duplicate_diagnostics'] = duplicate_diagnostics
    record['candidate_opening'] = opening_diagnostics
    record['reseeded_candidates'] = reseeded_candidates
    record['checkpoint_eligible'] = checkpoint_eligible
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
    candidates = self.config.model.candidates
    activation_sum = torch.zeros(candidates, device=self.device)
    activation_cross_product = torch.zeros((candidates, candidates), device=self.device)
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
        activations = output.segment_output.activations
        activation_sum += torch.sum(activations, dim=0)
        activation_cross_product += activations.transpose(0, 1) @ activations
        rows += len(batch.labels)
    if not rows:
      raise RuntimeError('validation dataset is empty')
    return ValidationResult(
      total_loss / rows,
      activation_sum.cpu(),
      activation_cross_product.cpu(),
      rows)

  def _candidate_duplicate_diagnostics(
      self,
      temperatures,
      activation_sum,
      activation_cross_product,
      rows):
    with torch.no_grad():
      all_url_gates = self.model.segment_layer.membership.gates(temperatures['url'])
      active_candidate_ids = torch.nonzero(
        self.model.candidate_mask_layer.candidate_mask,
        as_tuple=False).flatten()
      history_url_ids = torch.as_tensor(
        self.dataset.history_url_ids,
        dtype=torch.long,
        device=all_url_gates.device)
      observed_url_ids = torch.as_tensor(
        self.dataset.url_bucket_ids,
        dtype=torch.long,
        device=all_url_gates.device)
      history_url_gates = all_url_gates.index_select(
        0,
        active_candidate_ids)[:, history_url_ids]
      observed_url_gates = all_url_gates[:, observed_url_ids].cpu().numpy()
      url_jaccard = soft_jaccard_matrix(history_url_gates).cpu().numpy()
      activation_indices = active_candidate_ids.to(activation_sum.device)
      activation_similarity = activation_similarity_matrix(
        activation_sum.index_select(0, activation_indices),
        activation_cross_product.index_select(0, activation_indices).index_select(
          1,
          activation_indices),
        rows).cpu().numpy()
      forest_importance = self.model.forest.feature_importance(
        temperatures['forest_feature'],
        self.model.forest_feature_availability())[:self.config.model.candidates].cpu().numpy()
    active_candidate_ids = active_candidate_ids.cpu().numpy()
    first_local, second_local = numpy.triu_indices(len(active_candidate_ids), k=1)
    first = active_candidate_ids[first_local]
    second = active_candidate_ids[second_local]
    url_values = url_jaccard[first_local, second_local]
    activation_values = activation_similarity[first_local, second_local]
    top_pairs = []
    if len(first):
      pair_scores = numpy.minimum(url_values, activation_values)
      for pair_index in numpy.argsort(-pair_scores, kind='stable')[:10]:
        first_candidate = int(first[pair_index])
        second_candidate = int(second[pair_index])
        top_pairs.append({
          'candidate_i': first_candidate,
          'candidate_j': second_candidate,
          'url_jaccard': float(url_values[pair_index]),
          'activation_similarity': float(activation_values[pair_index]),
          'top_urls_i': self._top_candidate_urls(
            observed_url_gates,
            first_candidate),
          'top_urls_j': self._top_candidate_urls(
            observed_url_gates,
            second_candidate),
          'forest_importance_i': float(forest_importance[first_candidate]),
          'forest_importance_j': float(forest_importance[second_candidate]),
        })
    duplicate_pairs = [
      (int(first[index]), int(second[index]))
      for index in range(len(first))
      if (
        url_values[index] > self.config.loss.candidate_reseed_jaccard_threshold and
        activation_values[index] > self.config.loss.candidate_reseed_activation_threshold)
    ]
    diagnostics = {
      'max_pairwise_url_jaccard': _maximum_or_zero(url_values),
      'mean_pairwise_url_jaccard': _mean_or_zero(url_values),
      'max_activation_similarity': _maximum_or_zero(activation_values),
      'mean_activation_similarity': _mean_or_zero(activation_values),
      'number_of_pairs_jaccard_above_0_8': int(numpy.sum(url_values > 0.8)),
      'number_of_pairs_jaccard_above_0_95': int(numpy.sum(url_values > 0.95)),
      'number_of_activation_duplicate_pairs': int(numpy.sum(
        activation_values > self.config.loss.duplicate_activation_margin)),
      'number_of_reseed_duplicate_pairs': len(duplicate_pairs),
      'most_similar_pairs': top_pairs,
      'forest_importance': forest_importance.tolist(),
    }
    return diagnostics, duplicate_pairs

  def _top_candidate_urls(self, observed_url_gates, candidate, limit=5):
    top_url_ids = numpy.argsort(-observed_url_gates[candidate], kind='stable')[:limit]
    return [
      {
        'url': self.dataset.urls[int(url_id)],
        'gate': float(observed_url_gates[candidate, url_id]),
      }
      for url_id in top_url_ids
    ]

  def _candidate_opening_diagnostics(self, temperatures, opening_state):
    with torch.no_grad():
      observed_bucket_ids = torch.as_tensor(
        self.dataset.url_bucket_ids,
        dtype=torch.long,
        device=self.device)
      url_logits = self.model.segment_layer.membership.url_logits.index_select(
        1,
        observed_bucket_ids).cpu().numpy()
      url_gates = self.model.segment_layer.membership.gates(
        temperatures['url']).index_select(1, observed_bucket_ids).cpu().numpy()
      availability = self.model.forest_feature_availability()
      forest_importance = self.model.forest.feature_importance(
        temperatures['forest_feature'],
        availability)[:self.config.model.candidates].cpu().numpy()
      selected_features = self.model.forest.selected_feature_indices(
        availability).cpu().numpy()
    candidates = []
    for candidate in range(self.config.model.candidates):
      selected_urls = numpy.flatnonzero(url_logits[candidate] > 0)
      candidates.append({
        'index': candidate,
        'opened': bool(opening_state.candidate_mask[candidate]),
        'epoch_opened': opening_state.epoch_opened[candidate],
        'learning_rate_multiplier': opening_state.learning_rate_multipliers[candidate],
        'extracted_urls': [self.dataset.urls[int(url_id)] for url_id in selected_urls],
        'url_count': len(selected_urls),
        'top_url_gates': self._top_candidate_urls(url_gates, candidate, 10),
        'forest_soft_importance': float(forest_importance[candidate]),
        'forest_hard_split_count': int(numpy.sum(selected_features == candidate)),
      })
    return {
      'enabled': self.config.candidate_opening.enabled,
      'mode': self.config.candidate_opening.mode,
      'candidate_mask': [int(value) for value in opening_state.candidate_mask],
      'active_candidates': opening_state.active_candidates,
      'all_candidates_open': opening_state.all_candidates_open,
      'joint_finetune_active': opening_state.joint_finetune_active,
      'joint_finetune_complete': opening_state.joint_finetune_complete,
      'candidates': candidates,
    }

  def _reseed_duplicate_candidates(self, duplicate_pairs, forest_importance, epoch):
    candidates = []
    for component in _duplicate_components(duplicate_pairs):
      strongest = max(component, key=lambda value: (forest_importance[value], -value))
      candidates.extend(candidate for candidate in component if candidate != strongest)
    reseeded = []
    for candidate in sorted(set(candidates)):
      if candidate in self.reseeded_candidates:
        continue
      self._reseed_candidate(candidate, epoch)
      self.reseeded_candidates.add(candidate)
      reseeded.append(candidate)
    return reseeded

  def _reseed_candidate(self, candidate, epoch):
    membership_config = self.config.model.membership
    generator = numpy.random.default_rng(
      self.config.training.seed + (epoch + 1) * 1000003 + candidate)
    self._reseed_candidate_choice(
      self.model.segment_layer.window_logits,
      candidate,
      generator)
    self._reseed_candidate_choice(
      self.model.segment_layer.threshold_logits,
      candidate,
      generator)
    parameter = self.model.segment_layer.membership.url_logits
    observed_buckets = numpy.asarray(self.dataset.history_url_ids, dtype=numpy.int64)
    observed_indices = torch.as_tensor(
      observed_buckets,
      dtype=torch.long,
      device=parameter.device)
    with torch.no_grad():
      selected_elsewhere = torch.any(parameter[:, observed_indices] > 0, dim=0).cpu().numpy()
      parameter[candidate].normal_(
        membership_config.unselected_logit,
        membership_config.logit_std)
    self._reset_optimizer_candidate(parameter, candidate)
    if membership_config.initialization == 'symmetric_with_noise':
      return
    available = observed_buckets[~selected_elsewhere]
    if not len(available):
      available = observed_buckets
    urls_per_candidate = membership_config.initial_urls_per_candidate
    if membership_config.initialization == 'random_single_seed':
      urls_per_candidate = 1
    if membership_config.initialization == 'frequency' and self.dataset.history_counts is not None:
      frequencies = numpy.sum(
        self.dataset.history_counts[self.dataset.train_indices, :, -1],
        axis=0)
      frequency_by_bucket = dict(zip(observed_buckets.tolist(), frequencies.tolist()))
      selected_urls = numpy.asarray(sorted(
        available,
        key=lambda bucket: -frequency_by_bucket[int(bucket)])[:urls_per_candidate])
    else:
      selected_urls = generator.choice(
        available,
        urls_per_candidate,
        replace=urls_per_candidate > len(available))
    selected_indices = torch.as_tensor(
      selected_urls,
      dtype=torch.long,
      device=parameter.device)
    with torch.no_grad():
      parameter[candidate, selected_indices].normal_(
        membership_config.selected_logit,
        membership_config.logit_std)

  def _reseed_candidate_choice(self, parameter, candidate, generator):
    if parameter is None:
      return
    selected = int(generator.integers(parameter.shape[1]))
    with torch.no_grad():
      parameter[candidate].zero_()
      parameter[candidate, selected] = self.config.model.choice_initial_logit
    self._reset_optimizer_candidate(parameter, candidate)

  def _reset_optimizer_candidate(self, parameter, candidate):
    for value in self.optimizer.state.get(parameter, {}).values():
      if isinstance(value, torch.Tensor) and value.shape == parameter.shape:
        value[candidate].zero_()

  def _diagnostics(self, rules, soft_activations):
    temperatures = self.result_temperatures or self.temperatures(
      self.result_temperature_progress,
      self.result_candidate_opening_state)
    activation_tensor = torch.as_tensor(soft_activations)
    duplicate_diagnostics, _ = self._candidate_duplicate_diagnostics(
      temperatures,
      torch.sum(activation_tensor, dim=0),
      activation_tensor.transpose(0, 1) @ activation_tensor,
      len(activation_tensor))
    with torch.no_grad():
      url_gates = self.model.segment_layer.membership.gates(temperatures['url']).cpu().numpy()
      window_gates = self.model.segment_layer.window_gates(
        temperatures['window']).cpu().numpy()
      threshold_gates = self.model.segment_layer.threshold_gates(
        temperatures['threshold']).cpu().numpy()
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
        'forest_importance': duplicate_diagnostics['forest_importance'][candidate],
        'url_count': len(rule.url_ids),
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
      'candidate_duplicates': duplicate_diagnostics,
      'candidate_opening': self._candidate_opening_diagnostics(
        temperatures,
        self.result_candidate_opening_state),
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


def _duplicate_regularization_scale(epoch, batch_index, batches_per_epoch, start_epoch,
                                    ramp_epochs):
  position = epoch + batch_index / batches_per_epoch
  if position < start_epoch:
    return 0.0
  if ramp_epochs == 0:
    return 1.0
  return min((position - start_epoch) / ramp_epochs, 1.0)


def _duplicate_regularization_enabled(config):
  return config.loss.url_duplicate > 0 or config.loss.activation_duplicate > 0


def _duplicate_components(pairs):
  neighbors = {}
  for first, second in pairs:
    neighbors.setdefault(first, set()).add(second)
    neighbors.setdefault(second, set()).add(first)
  result = []
  visited = set()
  for candidate in sorted(neighbors):
    if candidate in visited:
      continue
    component = set()
    pending = [candidate]
    while pending:
      current = pending.pop()
      if current in component:
        continue
      component.add(current)
      pending.extend(neighbors.get(current, ()))
    visited.update(component)
    result.append(sorted(component))
  return result


def _maximum_or_zero(values):
  return float(numpy.max(values)) if len(values) else 0.0


def _mean_or_zero(values):
  return float(numpy.mean(values)) if len(values) else 0.0


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
