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
      selected_buckets = rule.url_bucket_ids if rule.url_bucket_ids else rule.url_ids
      if selected_buckets:
        url_masks[rule_index, list(selected_buckets)] = True
      selected_windows[rule_index] = window_indices[rule.window_seconds]
      thresholds[rule_index] = rule.min_visits
    self.aggregation = aggregation
    self.register_buffer('url_masks', url_masks)
    self.register_buffer('selected_windows', selected_windows)
    self.register_buffer('thresholds', thresholds)

  def forward(self, history_counts, history_url_ids=None):
    if history_counts.ndim != 3:
      raise ValueError('history_counts have an unexpected shape')
    url_masks = self.url_masks
    if history_url_ids is not None:
      url_masks = url_masks[:, history_url_ids]
    if history_counts.shape[1] != url_masks.shape[1]:
      raise ValueError('history_counts do not match active URL buckets')
    activations = []
    for segment_index in range(url_masks.shape[0]):
      url_mask = url_masks[segment_index]
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
