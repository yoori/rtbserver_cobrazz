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


class SegmentModelTrainer:
  def __init__(self, config, dataset):
    self.config = config
    self.dataset = dataset
    torch.manual_seed(config.training.seed)
    numpy.random.seed(config.training.seed)
    self.device = torch.device(config.training.device)
    self.model = SegmentCTRModel(
      config,
      len(dataset.urls),
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
    builder = SyntheticBatchBuilder(
      self.dataset,
      self.dataset.train_indices,
      self.config.data.batch_size,
      self.config.training.seed)
    total_epochs = self.config.training.discovery_epochs + self.config.training.structuring_epochs
    total_batches = total_epochs * builder.batches_per_epoch
    completed_batches = 0
    history = []
    for stage, epochs, include_existing in (
        ('discovery', self.config.training.discovery_epochs, False),
        ('structuring', self.config.training.structuring_epochs, True),
    ):
      requests = (
        BatchRequest(epoch, batch_index, include_existing)
        for epoch in range(epochs)
        for batch_index in range(builder.batches_per_epoch)
      )
      stage_epoch = 0
      epoch_values = []
      with ForkedBatchPool(
          builder,
          requests,
          self.config.data.batch_workers,
          self.config.data.ready_batches,
          self.config.data.batch_start_method) as batches:
        for batch_index, batch in enumerate(batches):
          epoch = batch_index // builder.batches_per_epoch
          if epoch != stage_epoch:
            self._record_epoch(history, stage, stage_epoch, epoch_values)
            stage_epoch = epoch
            epoch_values = []
          progress = completed_batches / max(1, total_batches - 1)
          epoch_values.append(self._train_batch(batch, self.temperatures(progress)))
          completed_batches += 1
      self._record_epoch(history, stage, stage_epoch, epoch_values)
      if checkpoint_dir is not None:
        self._save_checkpoint(checkpoint_dir / ('checkpoint-' + stage + '.pt'), stage)
    return history

  def evaluate(self):
    self.model.eval()
    rules = extract_segment_rules(self.model, self.dataset.urls)
    hard_segment_layer = HardSegmentLayer(
      rules,
      len(self.dataset.urls),
      self.config.data.windows_seconds,
      self.config.model.aggregation).to(self.device)
    builder = SyntheticBatchBuilder(
      self.dataset,
      self.dataset.validation_indices,
      self.config.data.batch_size,
      self.config.training.seed + 100000)
    requests = (
      BatchRequest(0, batch_index, True)
      for batch_index in range(builder.batches_per_epoch)
    )
    temperatures = self.temperatures(1.0)
    labels = []
    soft_probabilities = []
    hard_probabilities = []
    soft_activations = []
    hard_activations = []
    existing_channels = []
    sample_indices = []
    with torch.no_grad(), ForkedBatchPool(
        builder,
        requests,
        self.config.data.batch_workers,
        self.config.data.ready_batches,
        self.config.data.batch_start_method) as batches:
      for batch in batches:
        tensors = self._batch_tensors(batch)
        output = self.model(*tensors[:3], temperatures)
        hard = hard_segment_layer(tensors[0])
        hard_logits = self.model.segment_logits(hard, tensors[2], temperatures)
        labels.append(tensors[3].cpu().numpy())
        soft_probabilities.append(torch.sigmoid(output.logits).cpu().numpy())
        hard_probabilities.append(torch.sigmoid(hard_logits).cpu().numpy())
        soft_activations.append(output.segment_output.activations.cpu().numpy())
        hard_activations.append(hard.cpu().numpy())
        existing_channels.append(tensors[1].cpu().numpy())
        sample_indices.append(batch.sample_indices)
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
    if self.dataset.group_ids is not None:
      metrics['groups'] = _group_metrics(
        labels,
        soft_probabilities,
        hard_probabilities,
        self.dataset.group_ids[sample_indices],
        self.dataset.group_names)
    return metrics, rules

  def temperatures(self, progress):
    return {
      name: scheduler.value(progress)
      for name, scheduler in self.temperature_schedulers.items()
    }

  def _train_batch(self, batch, temperatures):
    self.model.train()
    tensors = self._batch_tensors(batch)
    self.optimizer.zero_grad(set_to_none=True)
    output = self.model(*tensors[:3], temperatures)
    loss = segment_model_loss(output, tensors[3], self.config)
    loss.total.backward()
    self.optimizer.step()
    return {
      **{
        field.name: float(getattr(loss, field.name).detach().cpu())
        for field in dataclasses.fields(loss)
      },
      'average_segment_activation': output.segment_output.activations.detach().mean(
        dim=0).cpu().tolist(),
      'temperatures': temperatures,
    }

  def _batch_tensors(self, batch):
    return (
      torch.from_numpy(batch.history_counts).to(self.device),
      torch.from_numpy(batch.existing_channels).to(self.device),
      torch.from_numpy(batch.context_features).to(self.device),
      torch.from_numpy(batch.labels).to(self.device),
    )

  def _initialize_membership(self):
    membership_config = self.config.model.membership
    if membership_config.initialization == 'random':
      return
    frequencies = numpy.sum(self.dataset.history_counts[self.dataset.train_indices, :, -1], axis=0)
    url_ranking = numpy.argsort(-frequencies, kind='stable')
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
      'existing_channel_ids': [rule.channel_id for rule in self.dataset.existing_rules],
    })
    if self.dataset.source_metadata is not None:
      _write_json(output_dir / 'source.json', self.dataset.source_metadata)

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

  def _record_epoch(self, history, stage, epoch, values):
    record = _epoch_record(stage, epoch, values)
    history.append(record)
    print(json.dumps(record, sort_keys=True), flush=True)

  def _diagnostics(self, rules, soft_activations):
    temperatures = self.temperatures(1.0)
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
    for candidate, rule in enumerate(rules):
      top_url_ids = numpy.argsort(-url_gates[candidate])[:10]
      candidate_diagnostics.append({
        'segment_id': candidate,
        'top_url_gates': [
          {
            'url': self.dataset.urls[int(url_id)],
            'gate': float(url_gates[candidate, url_id]),
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
      'url_gate_fraction_near_zero': float(numpy.mean(url_gates <= 0.1)),
      'url_gate_fraction_near_one': float(numpy.mean(url_gates >= 0.9)),
      'url_gate_fraction_ambiguous': float(numpy.mean((url_gates > 0.1) & (url_gates < 0.9))),
      'candidates': candidate_diagnostics,
    }


def _epoch_record(stage, epoch, values):
  return {
    'stage': stage,
    'epoch': epoch,
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


def _write_json(output_file, value):
  temporary_file = output_file.with_name(output_file.name + '.tmp')
  temporary_file.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n', encoding='utf-8')
  temporary_file.replace(output_file)


def _group_metrics(labels, soft_probabilities, hard_probabilities, group_ids, group_names):
  result = {}
  for group_id, group_name in enumerate(group_names):
    selected = group_ids == group_id
    group_labels = labels[selected]
    group_soft = soft_probabilities[selected]
    group_hard = hard_probabilities[selected]
    result[group_name] = {
      'rows': int(numpy.sum(selected)),
      'clicks': int(numpy.sum(group_labels)),
      'actual_ctr': float(numpy.mean(group_labels)),
      'average_soft_ctr': float(numpy.mean(group_soft)),
      'average_hard_ctr': float(numpy.mean(group_hard)),
      'soft_ctr': ctr_metrics(group_labels, group_soft),
      'hard_ctr': ctr_metrics(group_labels, group_hard),
    }
  return result
