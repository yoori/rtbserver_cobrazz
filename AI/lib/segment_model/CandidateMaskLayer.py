import torch


class CandidateMaskLayer(torch.nn.Module):
  def __init__(self, candidates, active_candidates):
    super().__init__()
    if candidates <= 0:
      raise ValueError('candidates must be positive')
    if not 0 <= active_candidates <= candidates:
      raise ValueError('active_candidates must be inside the candidate range')
    mask = torch.arange(candidates) < active_candidates
    self.register_buffer('candidate_mask', mask)

  def forward(self, candidate_activations):
    if candidate_activations.shape[-1] != len(self.candidate_mask):
      raise ValueError('candidate activations have an unexpected shape')
    return candidate_activations * self.candidate_mask.to(candidate_activations.dtype)

  def set_mask(self, candidate_mask):
    mask = torch.as_tensor(
      candidate_mask,
      dtype=torch.bool,
      device=self.candidate_mask.device)
    if mask.shape != self.candidate_mask.shape:
      raise ValueError('candidate mask has an unexpected shape')
    self.candidate_mask.copy_(mask)

