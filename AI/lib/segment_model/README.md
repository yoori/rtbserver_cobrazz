# Differentiable URL segment prototype

This package is the dense reference implementation for learning production URL rules from a
CTR objective. Python source files use the repository's `UpperCamelCase.py` naming convention.

The only history-to-predictor interface is the vector of candidate segment activations. Each
candidate directly owns URL membership logits, a distribution over configured time windows and
a distribution over configured integer visit thresholds. `SegmentRuleExtractor.py` converts
those parameters without clustering or another learned post-processing model.

Candidate membership can be initialized randomly or from URL frequency in the training history.
Frequency initialization does not inspect clicks or validation rows; it provides the same practical
vocabulary warm start that a production-scale sparse implementation will need.

`DifferentiableRandomForest.py` is the CTR predictor. It contains complete differentiable binary
trees, random per-node feature subsets, soft feature selection, soft threshold routing, Poisson
online bootstrap weights and trainable leaf logits. Existing `channel_id` values never enter this
forest. They are all zero during discovery training; during structuring training they are passed
only to `SegmentRelationLayer.py` to diagnose duplicates, extensions, refinements and overlaps.

`ForkedBatchPool` accepts any picklable batch builder. It starts several forked workers and keeps
a bounded group of ready batches. After the training process consumes one batch, the pool queues
the next request immediately. A production builder can therefore fetch an RImpression slice and
its ExpressionMatcher navigation profiles inside worker processes without changing the trainer.

Run the synthetic experiment with:

```text
SegmentModelTrain.py --config SegmentModelConfig.example.json --output-dir OUTPUT
SegmentModelEvaluate.py --model-dir OUTPUT
SegmentModelExtract.py --model-dir OUTPUT
```

Evaluation constructs `HardSegmentLayer` from the extracted rules and reports soft/hard segment
agreement, activation difference, CTR logloss, ROC-AUC, PR-AUC and permutation-invariant recovery
metrics against synthetic ground truth.
