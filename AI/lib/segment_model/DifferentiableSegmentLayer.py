import dataclasses

import torch

from .SegmentMembership import DenseSegmentMembership
from .SegmentModelConfig import MembershipConfig


@dataclasses.dataclass
class SegmentLayerOutput:
  activations: torch.Tensor
  selected_counts: torch.Tensor
  url_gates: torch.Tensor
  window_gates: torch.Tensor
  threshold_gates: torch.Tensor


class DifferentiableSegmentLayer(torch.nn.Module):
  def __init__(self, candidates, urls, windows_seconds, n_values, aggregation='softmax_max',
               activation_boundary=0.5, membership_config=None, choice_initial_logit=2.0):
    super().__init__()
    if aggregation not in ('sum', 'softmax_max'):
      raise ValueError("aggregation must be 'sum' or 'softmax_max'")
    self.candidates = candidates
    self.urls = urls
    self.aggregation = aggregation
    self.activation_boundary = activation_boundary
    if membership_config is None:
      membership_config = MembershipConfig()
    self.membership = DenseSegmentMembership(candidates, urls, membership_config)
    self.window_logits = torch.nn.Parameter(torch.zeros(candidates, len(windows_seconds)))
    self.threshold_logits = torch.nn.Parameter(torch.zeros(candidates, len(n_values)))
    with torch.no_grad():
      window_indices = (torch.randperm(candidates) % len(windows_seconds)).unsqueeze(1)
      self.window_logits.scatter_(1, window_indices, choice_initial_logit)
      self.threshold_logits[:, 0] = choice_initial_logit
    self.register_buffer('windows_seconds', torch.as_tensor(windows_seconds, dtype=torch.long))
    self.register_buffer('n_values', torch.as_tensor(n_values, dtype=torch.float32))

  def forward(self, history_counts, temperatures):
    url_gates = self.membership.gates(temperatures['url'])
    window_gates = torch.softmax(self.window_logits / temperatures['window'], dim=1)
    threshold_gates = torch.softmax(self.threshold_logits / temperatures['threshold'], dim=1)
    segment_counts = self._aggregate(history_counts, url_gates, temperatures['aggregation'])
    selected_counts = torch.sum(segment_counts * window_gates.unsqueeze(0), dim=2)
    thresholds = threshold_gates @ self.n_values
    activation_margin = selected_counts - thresholds.unsqueeze(0) + self.activation_boundary
    activations = torch.sigmoid(activation_margin / temperatures['activation'])
    return SegmentLayerOutput(
      activations,
      selected_counts,
      url_gates,
      window_gates,
      threshold_gates)

  def hard_activations(self, history_counts):
    selected_urls = self.membership.hard_gates()
    window_indices = torch.argmax(self.window_logits, dim=1)
    threshold_indices = torch.argmax(self.threshold_logits, dim=1)
    result = []
    for segment_index in range(self.candidates):
      url_mask = selected_urls[segment_index]
      if not torch.any(url_mask):
        result.append(torch.zeros(
          history_counts.shape[0],
          dtype=history_counts.dtype,
          device=history_counts.device))
        continue
      counts = history_counts[:, url_mask, window_indices[segment_index]]
      if self.aggregation == 'sum':
        selected_count = torch.sum(counts, dim=1)
      else:
        selected_count = torch.max(counts, dim=1).values
      threshold = self.n_values[threshold_indices[segment_index]]
      result.append((selected_count >= threshold).to(history_counts.dtype))
    return torch.stack(result, dim=1)

  def _aggregate(self, history_counts, url_gates, aggregation_temperature):
    if history_counts.ndim != 3:
      raise ValueError('history_counts must have shape [batch, urls, windows]')
    if history_counts.shape[1] != self.urls:
      raise ValueError('history_counts URL dimension does not match membership')
    if self.aggregation == 'sum':
      return torch.einsum('buw,su->bsw', history_counts, url_gates)
    gated_counts = history_counts[:, None, :, :] * url_gates[None, :, :, None]
    scaled_counts = gated_counts / aggregation_temperature
    positive_counts = scaled_counts > 0
    safe_counts = scaled_counts.clamp_min(torch.finfo(scaled_counts.dtype).eps)
    log_expm1 = safe_counts + torch.log(-torch.expm1(-safe_counts))
    log_expm1 = torch.where(positive_counts, log_expm1, torch.full_like(log_expm1, -torch.inf))
    baseline = torch.zeros(
      (*log_expm1.shape[:2], 1, log_expm1.shape[3]),
      dtype=log_expm1.dtype,
      device=log_expm1.device)
    values = torch.cat((baseline, log_expm1), dim=2)
    return aggregation_temperature * torch.logsumexp(values, dim=2)
