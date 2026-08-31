import math


class TemperatureScheduler:
  def __init__(self, schedule):
    self.schedule = schedule

  def value(self, progress):
    progress = min(1.0, max(0.0, float(progress)))
    progress = (
      progress - self.schedule.start_progress
    ) / (self.schedule.end_progress - self.schedule.start_progress)
    progress = min(1.0, max(0.0, progress))
    if self.schedule.schedule == 'linear':
      return self.schedule.start + progress * (self.schedule.end - self.schedule.start)
    ratio = self.schedule.end / self.schedule.start
    return self.schedule.start * math.exp(math.log(ratio) * progress)
