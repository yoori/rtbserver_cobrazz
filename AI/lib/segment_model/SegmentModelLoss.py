import dataclasses

import torch

from .CandidateDuplicate import candidate_pairs
from .CandidateDuplicate import centered_activation_similarity
from .CandidateDuplicate import duplicate_margin_loss
from .CandidateDuplicate import soft_jaccard_similarity


@dataclasses.dataclass
class SegmentLoss:
  total: torch.Tensor
  ctr: torch.Tensor
  sparsity: torch.Tensor
  binarization: torch.Tensor
  url_duplicate: torch.Tensor
  activation_duplicate: torch.Tensor
  duplicate_existing: torch.Tensor


def segment_model_loss(
    output,
    labels,
    config,
    regularization_scale=1.0,
    duplicate_regularization_scale=1.0):
  if not 0 <= regularization_scale <= 1:
    raise ValueError('regularization_scale must be inside [0, 1]')
  if not 0 <= duplicate_regularization_scale <= 1:
    raise ValueError('duplicate_regularization_scale must be inside [0, 1]')
  ctr = torch.nn.functional.binary_cross_entropy_with_logits(output.logits, labels)
  candidate_mask = output.candidate_mask
  url_gates = output.segment_output.active_url_gates[candidate_mask]
  activations = output.segment_output.activations[:, candidate_mask]
  window_gates = output.segment_output.window_gates[candidate_mask]
  threshold_gates = output.segment_output.threshold_gates[candidate_mask]
  sparsity = torch.mean(url_gates)
  binarization = (
    torch.mean(url_gates * (1.0 - url_gates)) +
    torch.mean(window_gates * (1.0 - window_gates)) +
    torch.mean(threshold_gates * (1.0 - threshold_gates)))
  url_duplicate = url_gates.new_zeros(())
  activation_duplicate = url_gates.new_zeros(())
  duplicate_enabled = (
    duplicate_regularization_scale > 0 and
    (config.loss.url_duplicate > 0 or config.loss.activation_duplicate > 0))
  if duplicate_enabled:
    pairs = candidate_pairs(url_gates.shape[0], config.loss.duplicate_pairs, url_gates.device)
    if config.loss.url_duplicate > 0:
      url_duplicate = duplicate_margin_loss(
        soft_jaccard_similarity(url_gates, pairs),
        config.loss.duplicate_jaccard_margin)
    if config.loss.activation_duplicate > 0:
      activation_duplicate = duplicate_margin_loss(
        centered_activation_similarity(activations, pairs),
        config.loss.duplicate_activation_margin)
  duplicate_existing = output.relations.duplicate_loss
  total = (
    ctr +
    regularization_scale * (
      config.loss.sparsity * sparsity +
      config.loss.binarization * binarization +
      config.loss.duplicate_existing * duplicate_existing) +
    duplicate_regularization_scale * (
      config.loss.url_duplicate * url_duplicate +
      config.loss.activation_duplicate * activation_duplicate))
  return SegmentLoss(
    total,
    ctr,
    sparsity,
    binarization,
    url_duplicate,
    activation_duplicate,
    duplicate_existing)
