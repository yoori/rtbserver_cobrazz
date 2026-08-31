import dataclasses

import torch


@dataclasses.dataclass
class SegmentRelations:
  candidate_given_channel: torch.Tensor
  channel_given_candidate: torch.Tensor
  duplicate_loss: torch.Tensor


class SegmentRelationLayer(torch.nn.Module):
  def __init__(self, duplicate_threshold=0.95):
    super().__init__()
    self.duplicate_threshold = duplicate_threshold

  def forward(self, candidate_activations, existing_channels):
    if existing_channels.shape[1] == 0:
      empty = candidate_activations.new_zeros((candidate_activations.shape[1], 0))
      return SegmentRelations(empty, empty, candidate_activations.new_zeros(()))
    both = torch.mean(candidate_activations[:, :, None] * existing_channels[:, None, :], dim=0)
    candidate_mass = torch.mean(candidate_activations, dim=0)[:, None]
    channel_mass = torch.mean(existing_channels, dim=0)[None, :]
    epsilon = torch.finfo(candidate_activations.dtype).eps
    candidate_given_channel = both / channel_mass.clamp_min(epsilon)
    channel_given_candidate = both / candidate_mass.clamp_min(epsilon)
    duplicate_score = torch.minimum(candidate_given_channel, channel_given_candidate)
    present = (channel_mass > epsilon).to(candidate_activations.dtype)
    duplicate_loss = torch.mean(
      torch.relu(duplicate_score - self.duplicate_threshold).square() * present)
    return SegmentRelations(candidate_given_channel, channel_given_candidate, duplicate_loss)
