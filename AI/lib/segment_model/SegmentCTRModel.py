import dataclasses

import torch

from .DifferentiableRandomForest import DifferentiableRandomForest
from .DifferentiableSegmentLayer import DifferentiableSegmentLayer
from .SegmentRelationLayer import SegmentRelationLayer


@dataclasses.dataclass
class SegmentModelOutput:
  logits: torch.Tensor
  tree_logits: torch.Tensor
  segment_output: object
  relations: object


class SegmentCTRModel(torch.nn.Module):
  def __init__(self, config, urls, existing_channels):
    super().__init__()
    self.config = config
    self.segment_layer = DifferentiableSegmentLayer(
      config.model.candidates,
      urls,
      config.data.windows_seconds,
      config.data.n_values,
      config.model.aggregation,
      config.model.activation_boundary,
      config.model.membership,
      config.model.choice_initial_logit)
    self.relation_layer = SegmentRelationLayer(config.loss.duplicate_threshold)
    self.forest = DifferentiableRandomForest(
      config.model.candidates + config.model.context_size,
      config.model.forest,
      binary_features=config.model.candidates)
    self.existing_channels = existing_channels

  def forward(
      self,
      history_counts,
      existing_channels,
      context_features,
      temperatures,
      history_url_ids=None):
    segment_output = self.segment_layer(history_counts, temperatures, history_url_ids)
    relations = self.relation_layer(segment_output.activations, existing_channels)
    forest_features = torch.cat((segment_output.activations, context_features), dim=1)
    logits, tree_logits = self.forest(
      forest_features,
      temperatures['forest_feature'],
      temperatures['forest_split'],
      return_tree_logits=True)
    return SegmentModelOutput(logits, tree_logits, segment_output, relations)

  def hard_segment_logits(
      self,
      history_counts,
      context_features,
      temperatures,
      history_url_ids=None):
    activations = self.segment_layer.hard_activations(history_counts, history_url_ids)
    return self.segment_logits(activations, context_features, temperatures)

  def segment_logits(self, activations, context_features, temperatures):
    forest_features = torch.cat((activations, context_features), dim=1)
    return self.forest(
      forest_features,
      temperatures['forest_feature'],
      temperatures['forest_split'])
