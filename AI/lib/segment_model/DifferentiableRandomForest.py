import torch


class DifferentiableRandomForest(torch.nn.Module):
  def __init__(self, input_features, config, binary_features=0):
    super().__init__()
    if not 0 <= binary_features <= input_features:
      raise ValueError('binary_features must be inside the input feature range')
    self.input_features = input_features
    self.binary_features = binary_features
    self.trees = config.trees
    self.depth = config.depth
    self.nodes = 2 ** config.depth - 1
    self.leaves = 2 ** config.depth
    features_per_node = min(config.features_per_node, input_features)
    generator = torch.Generator()
    generator.manual_seed(config.seed)
    feature_indices = torch.empty(self.trees, self.nodes, features_per_node, dtype=torch.long)
    for tree_index in range(self.trees):
      for node_index in range(self.nodes):
        feature_indices[tree_index, node_index] = torch.randperm(
          input_features,
          generator=generator)[:features_per_node]
    self.register_buffer('feature_indices', feature_indices)
    self.global_bias = torch.nn.Parameter(torch.zeros(()))
    self.feature_logits = torch.nn.Parameter(torch.zeros(self.trees, self.nodes, features_per_node))
    self.split_thresholds = torch.nn.Parameter(torch.empty(self.trees, self.nodes))
    self.leaf_logits = torch.nn.Parameter(torch.zeros(self.trees, self.leaves))
    torch.nn.init.normal_(self.feature_logits, std=0.05)
    with torch.no_grad():
      selected_features = torch.randint(features_per_node, (self.trees, self.nodes, 1))
      initial_logits = torch.full_like(
        selected_features,
        config.feature_initial_logit,
        dtype=torch.float32)
      self.feature_logits.scatter_add_(2, selected_features, initial_logits)
    torch.nn.init.uniform_(self.split_thresholds, 0.25, 0.75)

  def forward(self, features, feature_temperature, split_temperature, hard=False,
              return_tree_logits=False):
    if features.ndim != 2 or features.shape[1] != self.input_features:
      raise ValueError('forest features have an unexpected shape')
    candidates = features[:, self.feature_indices]
    thresholds = torch.where(
      self.feature_indices < self.binary_features,
      self.split_thresholds.new_tensor(0.5),
      self.split_thresholds[:, :, None])
    if hard:
      option_right = (candidates >= thresholds[None, :, :, :]).to(features.dtype)
    else:
      option_right = torch.sigmoid(
        (candidates - thresholds[None, :, :, :]) / split_temperature)
    feature_gates = self._feature_gates(features.dtype, feature_temperature, hard)
    right = torch.sum(option_right * feature_gates[None, :, :, :], dim=3)
    path_probabilities = features.new_ones((features.shape[0], self.trees, 1))
    node_offset = 0
    for level in range(self.depth):
      nodes_at_level = 2 ** level
      level_right = right[:, :, node_offset:node_offset + nodes_at_level]
      path_probabilities = torch.stack((
        path_probabilities * (1.0 - level_right),
        path_probabilities * level_right,
      ), dim=3).reshape(features.shape[0], self.trees, -1)
      node_offset += nodes_at_level
    tree_logits = torch.sum(path_probabilities * self.leaf_logits[None, :, :], dim=2)
    logits = self.global_bias + torch.sum(tree_logits, dim=1)
    if return_tree_logits:
      return logits, tree_logits
    return logits

  def _feature_gates(self, dtype, feature_temperature, hard):
    if hard:
      selected_indices = torch.argmax(self.feature_logits, dim=2)
      gates = torch.nn.functional.one_hot(
        selected_indices,
        num_classes=self.feature_logits.shape[2]).to(dtype)
    else:
      gates = torch.softmax(self.feature_logits / feature_temperature, dim=2)
    return gates

  def selected_feature_indices(self):
    local_indices = torch.argmax(self.feature_logits, dim=2, keepdim=True)
    return torch.gather(self.feature_indices, 2, local_indices).squeeze(2)
