import torch


PAIR_CHUNK_SIZE = 64


def candidate_pairs(candidates, sampled_pairs, device):
  if candidates < 2:
    return torch.empty((0, 2), dtype=torch.long, device=device)
  pairs = torch.triu_indices(candidates, candidates, offset=1, device=device).transpose(0, 1)
  if sampled_pairs > 0 and sampled_pairs < len(pairs):
    indices = torch.randperm(len(pairs), device=device)[:sampled_pairs]
    pairs = pairs[indices]
  return pairs


def soft_jaccard_similarity(url_gates, pairs):
  if not len(pairs):
    return url_gates.new_empty((0,))
  similarities = []
  epsilon = torch.finfo(url_gates.dtype).eps
  for pair_chunk in pairs.split(PAIR_CHUNK_SIZE):
    first = url_gates[pair_chunk[:, 0]]
    second = url_gates[pair_chunk[:, 1]]
    intersection = torch.sum(first * second, dim=1)
    union = torch.sum(first + second - first * second, dim=1)
    similarities.append(intersection / union.clamp_min(epsilon))
  return torch.cat(similarities)


def centered_activation_similarity(activations, pairs):
  if not len(pairs):
    return activations.new_empty((0,))
  centered = activations - torch.mean(activations, dim=0, keepdim=True)
  first = centered[:, pairs[:, 0]].transpose(0, 1)
  second = centered[:, pairs[:, 1]].transpose(0, 1)
  return torch.abs(torch.nn.functional.cosine_similarity(first, second, dim=1, eps=1e-8))


def duplicate_margin_loss(similarities, margin):
  if not len(similarities):
    return similarities.new_zeros(())
  return torch.mean(torch.relu(similarities - margin).square())


def soft_jaccard_matrix(url_gates):
  intersection = url_gates @ url_gates.transpose(0, 1)
  mass = torch.sum(url_gates, dim=1)
  union = mass[:, None] + mass[None, :] - intersection
  epsilon = torch.finfo(url_gates.dtype).eps
  return intersection / union.clamp_min(epsilon)


def activation_similarity_matrix(activation_sum, activation_cross_product, rows):
  if rows <= 0:
    raise ValueError('activation statistics must contain rows')
  centered = activation_cross_product - torch.outer(activation_sum, activation_sum) / rows
  variance = torch.diagonal(centered).clamp_min(0.0)
  denominator = torch.sqrt(variance[:, None] * variance[None, :])
  epsilon = torch.finfo(centered.dtype).eps
  similarity = torch.where(
    denominator > epsilon,
    torch.abs(centered / denominator.clamp_min(epsilon)),
    torch.zeros_like(centered))
  similarity.fill_diagonal_(1.0)
  return similarity
