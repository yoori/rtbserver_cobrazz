import torch


class DenseSegmentMembership(torch.nn.Module):
  def __init__(self, candidates, urls, config):
    super().__init__()
    self.candidates = candidates
    self.urls = urls
    self.url_logits = torch.nn.Parameter(torch.empty(candidates, urls))
    torch.nn.init.normal_(self.url_logits, mean=config.unselected_logit, std=config.logit_std)
    selected_urls = min(config.initial_urls_per_candidate, urls)
    with torch.no_grad():
      for candidate in range(candidates):
        indices = torch.randperm(urls)[:selected_urls]
        values = torch.empty(selected_urls)
        torch.nn.init.normal_(values, mean=config.selected_logit, std=config.logit_std)
        self.url_logits[candidate, indices] = values

  def gates(self, temperature):
    return torch.sigmoid(self.url_logits / temperature)

  def hard_gates(self):
    return self.url_logits > 0
