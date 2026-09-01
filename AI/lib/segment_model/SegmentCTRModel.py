import dataclasses

import torch

from .CandidateMaskLayer import CandidateMaskLayer
from .DifferentiableRandomForest import DifferentiableRandomForest
from .DifferentiableSegmentLayer import DifferentiableSegmentLayer
from .SegmentRelationLayer import SegmentRelationLayer


@dataclasses.dataclass
class SegmentModelOutput:
  logits: torch.Tensor
  tree_logits: torch.Tensor
  segment_output: object
  masked_candidate_activations: torch.Tensor
  candidate_mask: torch.Tensor
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
    active_candidates = (
      config.candidate_opening.first_active_candidates
      if config.candidate_opening.enabled else config.model.candidates)
    self.candidate_mask_layer = CandidateMaskLayer(
      config.model.candidates,
      active_candidates)
    self.relation_layer = SegmentRelationLayer(config.loss.duplicate_threshold)
    self.existing_channels = existing_channels
    self.forest = DifferentiableRandomForest(
      config.model.candidates + existing_channels + config.model.context_size,
      config.model.forest,
      binary_features=config.model.candidates + existing_channels)

  def forward(
      self,
      history_counts,
      existing_channels,
      context_features,
      temperatures,
      history_url_ids=None):
    segment_output = self.segment_layer(history_counts, temperatures, history_url_ids)
    candidate_mask = self.candidate_mask_layer.candidate_mask
    masked_activations = self.candidate_mask_layer(segment_output.activations)
    relations = self.relation_layer(
      segment_output.activations[:, candidate_mask],
      existing_channels)
    forest_features = torch.cat((masked_activations, existing_channels, context_features), dim=1)
    logits, tree_logits = self.forest(
      forest_features,
      temperatures['forest_feature'],
      temperatures['forest_split'],
      return_tree_logits=True,
      feature_availability=self.forest_feature_availability())
    return SegmentModelOutput(
      logits,
      tree_logits,
      segment_output,
      masked_activations,
      candidate_mask,
      relations)

  def hard_segment_logits(
      self,
      history_counts,
      existing_channels,
      context_features,
      temperatures,
      history_url_ids=None):
    activations = self.segment_layer.hard_activations(history_counts, history_url_ids)
    return self.segment_logits(activations, existing_channels, context_features, temperatures)

  def segment_logits(self, activations, existing_channels, context_features, temperatures):
    masked_activations = self.candidate_mask_layer(activations)
    forest_features = torch.cat((masked_activations, existing_channels, context_features), dim=1)
    return self.forest(
      forest_features,
      temperatures['forest_feature'],
      temperatures['forest_split'],
      feature_availability=self.forest_feature_availability())

  def set_candidate_mask(self, candidate_mask):
    self.candidate_mask_layer.set_mask(candidate_mask)

  def forest_feature_availability(self):
    unmasked_features = self.existing_channels + self.config.model.context_size
    return torch.cat((
      self.candidate_mask_layer.candidate_mask,
      torch.ones(
        unmasked_features,
        dtype=torch.bool,
        device=self.candidate_mask_layer.candidate_mask.device),
    ))
