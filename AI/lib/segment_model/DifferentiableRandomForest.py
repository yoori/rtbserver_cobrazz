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
    torch.nn.init.normal_(self.feature_logits, std=config.feature_logit_std)
    torch.nn.init.normal_(self.leaf_logits, std=config.leaf_logit_std)
    torch.nn.init.uniform_(self.split_thresholds, 0.25, 0.75)

  def forward(self, features, feature_temperature, split_temperature, hard=False,
              return_tree_logits=False, feature_availability=None):
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
    feature_gates = self._feature_gates(
      features.dtype,
      feature_temperature,
      hard,
      feature_availability)
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

  def _feature_gates(self, dtype, feature_temperature, hard, feature_availability=None):
    availability = self._feature_availability(feature_availability)
    local_availability = availability[self.feature_indices]
    masked_logits = self.feature_logits.masked_fill(~local_availability, -torch.inf)
    has_available_feature = torch.any(local_availability, dim=2, keepdim=True)
    safe_logits = torch.where(has_available_feature, masked_logits, torch.zeros_like(masked_logits))
    if hard:
      selected_indices = torch.argmax(safe_logits, dim=2)
      gates = torch.nn.functional.one_hot(
        selected_indices,
        num_classes=self.feature_logits.shape[2]).to(dtype)
      return gates * local_availability.to(dtype)
    else:
      gates = torch.softmax(safe_logits / feature_temperature, dim=2)
      return gates * local_availability.to(dtype)

  def selected_feature_indices(self, feature_availability=None):
    gates = self._feature_gates(
      self.feature_logits.dtype,
      1.0,
      True,
      feature_availability)
    local_indices = torch.argmax(gates, dim=2, keepdim=True)
    selected = torch.gather(self.feature_indices, 2, local_indices).squeeze(2)
    return torch.where(torch.any(gates > 0, dim=2), selected, torch.full_like(selected, -1))

  def feature_importance(self, feature_temperature, feature_availability=None):
    gates = self._feature_gates(
      self.feature_logits.dtype,
      feature_temperature,
      False,
      feature_availability)
    importance = gates.new_zeros(self.input_features)
    importance.scatter_add_(0, self.feature_indices.reshape(-1), gates.reshape(-1))
    return importance / (self.trees * self.nodes)

  def _feature_availability(self, feature_availability):
    if feature_availability is None:
      return torch.ones(
        self.input_features,
        dtype=torch.bool,
        device=self.feature_logits.device)
    availability = torch.as_tensor(
      feature_availability,
      dtype=torch.bool,
      device=self.feature_logits.device)
    if availability.shape != (self.input_features,):
      raise ValueError('forest feature availability has an unexpected shape')
    return availability
