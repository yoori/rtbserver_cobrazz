#!/usr/bin/python3.12

import importlib.util
import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))
TORCH_AVAILABLE = importlib.util.find_spec('torch') is not None


@unittest.skipUnless(TORCH_AVAILABLE, 'PyTorch is not installed in the source environment')
class SegmentModelLossTest(unittest.TestCase):
  @staticmethod
  def temperatures():
    return {
      'url': 1.0,
      'window': 1.0,
      'threshold': 1.0,
      'activation': 1.0,
      'aggregation': 1.0,
      'forest_feature': 1.0,
      'forest_split': 1.0,
    }

  def test_url_regularization_only_updates_observed_buckets(self):
    import torch
    from segment_model.SegmentCTRModel import SegmentCTRModel
    from segment_model.SegmentModelConfig import SegmentModelConfig
    from segment_model.SegmentModelLoss import segment_model_loss

    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [60],
        'n_values': [1],
        'url_buckets': 5,
        'batch_workers': 1,
        'ready_batches': 1,
      },
      'model': {
        'candidates': 1,
        'forest': {
          'trees': 1,
          'depth': 1,
          'features_per_node': 1,
        },
      },
      'loss': {
        'sparsity': 1.0,
        'binarization': 1.0,
        'url_duplicate': 0.0,
        'activation_duplicate': 0.0,
        'duplicate_existing': 0.0,
        'duplicate_pairs': 0,
      },
      'synthetic': {'true_segments': 1},
    })
    model = SegmentCTRModel(config, 5, 0)
    counts = torch.tensor([[[1.0], [0.0]], [[0.0], [1.0]]])
    active_url_ids = torch.tensor([1, 3])
    output = model(
      counts,
      torch.empty((2, 0)),
      torch.empty((2, 0)),
      self.temperatures(),
      active_url_ids)
    segment_model_loss(output, torch.tensor([0.0, 1.0]), config).total.backward()
    gradient = model.segment_layer.membership.url_logits.grad[0]
    self.assertGreater(float(torch.sum(torch.abs(gradient[active_url_ids]))), 0.0)
    self.assertEqual(0.0, float(torch.sum(torch.abs(gradient[[0, 2, 4]]))))

  def test_url_sparsity_has_the_same_coefficient_for_every_candidate(self):
    import torch
    from segment_model.SegmentCTRModel import SegmentCTRModel
    from segment_model.SegmentModelConfig import SegmentModelConfig
    from segment_model.SegmentModelLoss import segment_model_loss

    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [60],
        'n_values': [1],
        'url_buckets': 4,
        'batch_workers': 1,
        'ready_batches': 1,
      },
      'model': {
        'candidates': 2,
        'forest': {'trees': 1, 'depth': 1, 'features_per_node': 2},
      },
      'loss': {
        'sparsity': 1.0,
        'binarization': 0.0,
        'url_duplicate': 0.0,
        'activation_duplicate': 0.0,
        'duplicate_existing': 0.0,
      },
      'synthetic': {'true_segments': 1},
    })
    model = SegmentCTRModel(config, 4, 0)
    with torch.no_grad():
      model.segment_layer.membership.url_logits.fill_(-1.0)
    active_url_ids = torch.tensor([1, 3])
    output = model(
      torch.ones((1, 2, 1)),
      torch.empty((1, 0)),
      torch.empty((1, 0)),
      self.temperatures(),
      active_url_ids)
    loss = segment_model_loss(output, torch.tensor([0.0]), config)
    loss.sparsity.backward()
    gradient = model.segment_layer.membership.url_logits.grad
    self.assertTrue(torch.equal(gradient[0], gradient[1]))

  def test_closed_candidate_is_excluded_from_ctr_and_auxiliary_losses(self):
    import torch
    from segment_model.SegmentCTRModel import SegmentCTRModel
    from segment_model.SegmentModelConfig import SegmentModelConfig
    from segment_model.SegmentModelLoss import segment_model_loss

    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [60],
        'n_values': [1],
        'url_buckets': 4,
        'batch_workers': 1,
        'ready_batches': 1,
      },
      'model': {
        'candidates': 2,
        'forest': {'trees': 1, 'depth': 1, 'features_per_node': 2},
      },
      'loss': {
        'sparsity': 1.0,
        'binarization': 1.0,
        'url_duplicate': 1.0,
        'activation_duplicate': 1.0,
        'duplicate_existing': 1.0,
        'duplicate_regularization_start_epoch': 0,
        'duplicate_regularization_ramp_epochs': 0,
      },
      'training': {
        'discovery_epochs': 1,
        'structuring_epochs': 1,
        'max_epochs': 3,
      },
      'candidate_opening': {
        'enabled': True,
        'first_active_candidates': 1,
        'open_every_epochs': 1,
      },
      'synthetic': {'true_segments': 1},
    })
    model = SegmentCTRModel(config, 4, 1)
    output = model(
      torch.ones((2, 4, 1)),
      torch.tensor([[0.0], [1.0]]),
      torch.empty((2, 0)),
      self.temperatures(),
      torch.arange(4))
    loss = segment_model_loss(output, torch.tensor([0.0, 1.0]), config)
    loss.total.backward()
    gradient = model.segment_layer.membership.url_logits.grad
    self.assertGreater(float(torch.sum(torch.abs(gradient[0]))), 0.0)
    self.assertEqual(0.0, float(torch.sum(torch.abs(gradient[1]))))
    importance = model.forest.feature_importance(
      1.0,
      model.forest_feature_availability())
    self.assertEqual(0.0, float(importance[1]))
    self.assertEqual([True, False, True], model.forest_feature_availability().tolist())

  def test_duplicate_losses_ignore_partial_overlap_and_penalize_near_duplicates(self):
    import torch
    from segment_model.CandidateDuplicate import centered_activation_similarity
    from segment_model.CandidateDuplicate import duplicate_margin_loss
    from segment_model.CandidateDuplicate import soft_jaccard_similarity

    url_gates = torch.tensor([
      [1.0, 1.0, 0.0, 0.0],
      [1.0, 1.0, 0.0, 0.0],
      [1.0, 0.0, 1.0, 0.0],
    ])
    activations = torch.tensor([
      [0.0, 0.0, 0.0],
      [1.0, 1.0, 0.0],
      [0.0, 0.0, 1.0],
      [1.0, 1.0, 1.0],
    ])
    pairs = torch.tensor([[0, 1], [0, 2]])
    url_similarity = soft_jaccard_similarity(url_gates, pairs)
    activation_similarity = centered_activation_similarity(activations, pairs)
    self.assertAlmostEqual(1.0, float(url_similarity[0]))
    self.assertLess(float(url_similarity[1]), 0.8)
    self.assertAlmostEqual(1.0, float(activation_similarity[0]))
    self.assertLess(float(activation_similarity[1]), 0.9)
    self.assertGreater(float(duplicate_margin_loss(url_similarity[:1], 0.8)), 0.0)
    self.assertEqual(0.0, float(duplicate_margin_loss(url_similarity[1:], 0.8)))
    self.assertGreater(float(duplicate_margin_loss(activation_similarity[:1], 0.9)), 0.0)
    self.assertEqual(0.0, float(duplicate_margin_loss(activation_similarity[1:], 0.9)))

  def test_url_duplicate_penalty_can_separate_near_duplicate_candidates(self):
    import torch
    from segment_model.CandidateDuplicate import duplicate_margin_loss
    from segment_model.CandidateDuplicate import soft_jaccard_similarity

    logits = torch.nn.Parameter(torch.tensor([
      [3.0, 3.0, -3.0, -3.0],
      [3.01, 2.99, -3.01, -2.99],
    ]))
    pairs = torch.tensor([[0, 1]])
    optimizer = torch.optim.Adam([logits], lr=0.05)
    initial_similarity = None
    for _ in range(200):
      optimizer.zero_grad(set_to_none=True)
      similarity = soft_jaccard_similarity(torch.sigmoid(logits), pairs)
      if initial_similarity is None:
        initial_similarity = float(similarity.detach())
      loss = duplicate_margin_loss(similarity, 0.8)
      loss.backward()
      optimizer.step()
    final_similarity = float(
      soft_jaccard_similarity(torch.sigmoid(logits), pairs).detach())
    self.assertGreater(initial_similarity, 0.8)
    self.assertLessEqual(final_similarity, 0.801)


if __name__ == '__main__':
  unittest.main()
