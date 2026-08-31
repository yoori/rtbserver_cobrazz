import dataclasses

import numpy


@dataclasses.dataclass
class SegmentRelation:
  channel_id: int
  relation: str
  candidate_given_channel: float
  channel_given_candidate: float

  def to_dict(self):
    return dataclasses.asdict(self)


@dataclasses.dataclass
class SegmentRule:
  segment_id: int
  urls: list
  url_ids: tuple
  window_seconds: int
  min_visits: int
  predictor_weight: float | None = None
  forest_split_count: int = 0
  relations: list = dataclasses.field(default_factory=list)

  def to_dict(self):
    return {
      'segment_id': self.segment_id,
      'urls': self.urls,
      'window_seconds': self.window_seconds,
      'min_visits': self.min_visits,
      'predictor_weight': self.predictor_weight,
      'forest_split_count': self.forest_split_count,
      'relations': [relation.to_dict() for relation in self.relations],
    }


def extract_segment_rules(model, urls, hard_activations=None, existing_channels=None,
                          existing_channel_ids=None, relation_threshold=0.9):
  url_logits = model.segment_layer.membership.url_logits.detach().cpu().numpy()
  window_indices = model.segment_layer.window_logits.detach().argmax(dim=1).cpu().numpy()
  threshold_indices = model.segment_layer.threshold_logits.detach().argmax(dim=1).cpu().numpy()
  windows = model.segment_layer.windows_seconds.detach().cpu().numpy()
  n_values = model.segment_layer.n_values.detach().cpu().numpy()
  selected_features = model.forest.selected_feature_indices().detach().cpu().numpy()
  relation_values = None
  if hard_activations is not None and existing_channels is not None:
    relation_values = _relation_values(hard_activations, existing_channels)
  rules = []
  for segment_id in range(url_logits.shape[0]):
    url_ids = tuple(numpy.flatnonzero(url_logits[segment_id] > 0).tolist())
    relations = []
    if relation_values is not None:
      for channel_index in range(relation_values[0].shape[1]):
        candidate_given = float(relation_values[0][segment_id, channel_index])
        channel_given = float(relation_values[1][segment_id, channel_index])
        relation = _relation_type(candidate_given, channel_given, relation_threshold)
        if relation != 'independent':
          channel_id = (
            existing_channel_ids[channel_index]
            if existing_channel_ids is not None else channel_index)
          relations.append(SegmentRelation(
            int(channel_id),
            relation,
            candidate_given,
            channel_given))
    rules.append(SegmentRule(
      segment_id=segment_id,
      urls=[urls[url_id] for url_id in url_ids],
      url_ids=url_ids,
      window_seconds=int(windows[window_indices[segment_id]]),
      min_visits=int(n_values[threshold_indices[segment_id]]),
      forest_split_count=int(numpy.sum(selected_features == segment_id)),
      relations=relations))
  return rules


def _relation_values(candidate_activations, existing_channels):
  candidates = numpy.asarray(candidate_activations, dtype=numpy.float64)
  existing = numpy.asarray(existing_channels, dtype=numpy.float64)
  both = numpy.mean(candidates[:, :, None] * existing[:, None, :], axis=0)
  candidate_mass = numpy.mean(candidates, axis=0)[:, None]
  channel_mass = numpy.mean(existing, axis=0)[None, :]
  candidate_given = numpy.divide(
    both,
    channel_mass,
    out=numpy.zeros_like(both),
    where=channel_mass > 0)
  channel_given = numpy.divide(
    both,
    candidate_mass,
    out=numpy.zeros_like(both),
    where=candidate_mass > 0)
  return candidate_given, channel_given


def _relation_type(candidate_given, channel_given, threshold):
  if candidate_given >= threshold and channel_given >= threshold:
    return 'duplicate'
  if candidate_given >= threshold:
    return 'extends'
  if channel_given >= threshold:
    return 'refines'
  if candidate_given > 0.1 and channel_given > 0.1:
    return 'overlaps'
  return 'independent'
