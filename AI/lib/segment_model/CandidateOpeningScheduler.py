import dataclasses
import math


@dataclasses.dataclass(frozen=True)
class CandidateOpeningState:
  position: float
  candidate_mask: tuple
  epoch_opened: tuple
  learning_rate_multipliers: tuple
  active_candidates: int
  all_candidates_open: bool
  joint_finetune_active: bool
  joint_finetune_complete: bool


class CandidateOpeningScheduler:
  def __init__(self, config, candidates):
    self.config = config
    self.candidates = candidates
    if config.enabled:
      self.epoch_opened = tuple(
        0 if candidate < config.first_active_candidates else
        (candidate - config.first_active_candidates + 1) * config.open_every_epochs
        for candidate in range(candidates))
    else:
      self.epoch_opened = (0,) * candidates
    self.all_open_epoch = self.epoch_opened[-1]
    self.joint_finetune_end_epoch = self.all_open_epoch + config.joint_finetune_epochs

  def state(self, position):
    position = max(0.0, float(position))
    candidate_mask = tuple(position >= epoch for epoch in self.epoch_opened)
    active_candidates = sum(candidate_mask)
    all_candidates_open = active_candidates == self.candidates
    joint_finetune_active = (
      self.config.enabled and
      all_candidates_open and
      self.config.joint_finetune_epochs > 0 and
      position < self.joint_finetune_end_epoch)
    joint_finetune_complete = (
      not self.config.enabled or
      self.config.joint_finetune_epochs == 0 or
      position >= self.joint_finetune_end_epoch)
    multipliers = self._learning_rate_multipliers(
      position,
      candidate_mask,
      all_candidates_open)
    return CandidateOpeningState(
      position,
      candidate_mask,
      self.epoch_opened,
      multipliers,
      active_candidates,
      all_candidates_open,
      joint_finetune_active,
      joint_finetune_complete)

  def url_temperature(self, base_temperature, position, schedule):
    state = self.state(position)
    if not self.config.enabled:
      return base_temperature
    floor = self.config.url_temperature_floor
    if not state.all_candidates_open:
      return max(base_temperature, floor)
    if self.config.joint_finetune_epochs <= 0:
      return base_temperature
    progress = (
      position - self.all_open_epoch
    ) / self.config.joint_finetune_epochs
    progress = min(1.0, max(0.0, progress))
    start = max(base_temperature, floor)
    if schedule == 'linear':
      return start + progress * (base_temperature - start)
    if start == base_temperature:
      return base_temperature
    return start * math.exp(math.log(base_temperature / start) * progress)

  def _learning_rate_multipliers(self, position, candidate_mask, all_candidates_open):
    if not self.config.enabled:
      return (1.0,) * self.candidates
    if all_candidates_open and self.config.joint_finetune_epochs > 0:
      multiplier = self.config.joint_finetune_lr_multiplier
      return tuple(multiplier if opened else 0.0 for opened in candidate_mask)
    if self.config.previous_candidate_lr_mode == 'full':
      return tuple(1.0 if opened else 0.0 for opened in candidate_mask)
    latest_epoch = max(
      epoch
      for epoch, opened in zip(self.epoch_opened, candidate_mask)
      if opened)
    return tuple(
      0.0 if not opened else
      1.0 if epoch == latest_epoch else self.config.previous_candidate_lr_multiplier
      for epoch, opened in zip(self.epoch_opened, candidate_mask))
