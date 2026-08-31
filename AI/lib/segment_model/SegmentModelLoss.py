import dataclasses

import torch


@dataclasses.dataclass
class SegmentLoss:
  total: torch.Tensor
  ctr: torch.Tensor
  forest_bootstrap: torch.Tensor
  sparsity: torch.Tensor
  binarization: torch.Tensor
  diversity: torch.Tensor
  duplicate_existing: torch.Tensor


def segment_model_loss(output, labels, config):
  ctr = torch.nn.functional.binary_cross_entropy_with_logits(output.logits, labels)
  forest_bootstrap = _forest_bootstrap_loss(output.tree_logits, labels, config)
  url_gates = output.segment_output.url_gates
  window_gates = output.segment_output.window_gates
  threshold_gates = output.segment_output.threshold_gates
  sparsity = torch.mean(url_gates)
  binarization = (
    torch.mean(url_gates * (1.0 - url_gates)) +
    torch.mean(window_gates * (1.0 - window_gates)) +
    torch.mean(threshold_gates * (1.0 - threshold_gates)))
  diversity = _diversity_loss(url_gates, config.loss.diversity_pairs)
  duplicate_existing = output.relations.duplicate_loss
  total = (
    ctr +
    config.model.forest.bootstrap_loss * forest_bootstrap +
    config.loss.sparsity * sparsity +
    config.loss.binarization * binarization +
    config.loss.diversity * diversity +
    config.loss.duplicate_existing * duplicate_existing)
  return SegmentLoss(
    total,
    ctr,
    forest_bootstrap,
    sparsity,
    binarization,
    diversity,
    duplicate_existing)


def _forest_bootstrap_loss(tree_logits, labels, config):
  if config.model.forest.bootstrap == 'none':
    return tree_logits.new_zeros(())
  weights = torch.poisson(torch.ones_like(tree_logits))
  losses = torch.nn.functional.binary_cross_entropy_with_logits(
    tree_logits,
    labels[:, None].expand_as(tree_logits),
    reduction='none')
  return torch.sum(losses * weights) / torch.sum(weights).clamp_min(1.0)


def _diversity_loss(url_gates, sampled_pairs):
  candidates = url_gates.shape[0]
  if candidates < 2 or sampled_pairs <= 0:
    return url_gates.new_zeros(())
  first = torch.randint(0, candidates, (sampled_pairs,), device=url_gates.device)
  offset = torch.randint(1, candidates, (sampled_pairs,), device=url_gates.device)
  second = (first + offset) % candidates
  similarity = torch.nn.functional.cosine_similarity(
    url_gates[first],
    url_gates[second],
    dim=1,
    eps=1e-8)
  return torch.mean(similarity.square())
