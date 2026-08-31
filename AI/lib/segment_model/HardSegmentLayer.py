import torch


class HardSegmentLayer(torch.nn.Module):
  def __init__(self, rules, url_count, windows_seconds, aggregation='softmax_max'):
    super().__init__()
    if aggregation not in ('sum', 'softmax_max'):
      raise ValueError("aggregation must be 'sum' or 'softmax_max'")
    window_indices = {int(value): index for index, value in enumerate(windows_seconds)}
    url_masks = torch.zeros((len(rules), url_count), dtype=torch.bool)
    selected_windows = torch.empty(len(rules), dtype=torch.long)
    thresholds = torch.empty(len(rules), dtype=torch.float32)
    for rule_index, rule in enumerate(rules):
      if rule.window_seconds not in window_indices:
        raise ValueError('rule contains an unsupported time window')
      if rule.url_ids:
        url_masks[rule_index, list(rule.url_ids)] = True
      selected_windows[rule_index] = window_indices[rule.window_seconds]
      thresholds[rule_index] = rule.min_visits
    self.aggregation = aggregation
    self.register_buffer('url_masks', url_masks)
    self.register_buffer('selected_windows', selected_windows)
    self.register_buffer('thresholds', thresholds)

  def forward(self, history_counts):
    if history_counts.ndim != 3 or history_counts.shape[1] != self.url_masks.shape[1]:
      raise ValueError('history_counts have an unexpected shape')
    activations = []
    for segment_index in range(self.url_masks.shape[0]):
      url_mask = self.url_masks[segment_index]
      if not torch.any(url_mask):
        activations.append(history_counts.new_zeros(history_counts.shape[0]))
        continue
      counts = history_counts[:, url_mask, self.selected_windows[segment_index]]
      if self.aggregation == 'sum':
        selected_count = torch.sum(counts, dim=1)
      else:
        selected_count = torch.max(counts, dim=1).values
      activations.append(
        (selected_count >= self.thresholds[segment_index]).to(history_counts.dtype))
    return torch.stack(activations, dim=1)
