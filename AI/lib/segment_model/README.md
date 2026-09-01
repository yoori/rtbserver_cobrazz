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

URLs are mapped to a fixed number of input buckets with `CRC32(url) % url_buckets`; the default is
one million buckets. Membership therefore has a stable `candidates x url_buckets` parameter shape.
A batch carries only its observed bucket IDs and counts, rather than a dense
`batch x url_buckets` tensor. Training writes `url-bucket-dictionary.json` so every selected bucket
can be expanded back to all observed URLs, including hash collisions.

`DifferentiableRandomForest.py` is the CTR predictor. It contains complete differentiable binary
trees, random per-node feature subsets, soft feature selection, soft threshold routing and
trainable leaf contributions. The forest raw result is a trainable global bias plus the sum of all
tree contributions. Existing `channel_id` values never enter this forest. They are all zero during
discovery training; during structuring training they are passed only to `SegmentRelationLayer.py`
to diagnose duplicates, extensions, refinements and overlaps.

`ForkedBatchPool` accepts any picklable batch builder. It starts several forked workers and keeps
a bounded group of ready batches. After the training process consumes one batch, the pool queues
the next request immediately. A production builder can therefore fetch an RImpression slice and
its ExpressionMatcher navigation profiles inside worker processes without changing the trainer.

`RImpressionScenarioData.py` supplies the first such external builder. It reads ordered
RImpression ranges from ClickHouse, posts each user and impression date to ExpressionMatcher and
constructs window counts from earlier navigation days. The companion compose project in
`AI/segment_model_scenario` provides deterministic ClickHouse and ExpressionMatcher mocks for the
correlated-URL and URL-union experiments.

Repeat `--expression-matcher-url` to configure sharded ExpressionMatcher hosts. Users from one
RImpression batch are grouped by impression day, and each complete daily user list is sent to every
host in parallel. The trainer merges only returned users and does not contain shard-routing logic.

Training runs for at most `max_epochs`, validates after every epoch and restores the best
checkpoint. It stops after `early_stopping_patience` epochs without an improvement larger than
`early_stopping_min_delta`; the default delta is `1e-6` so numerical tail improvements do not
reset patience.

URL regularization is evaluated only on buckets observed by the dataset. During discovery its
weight is zero and all temperatures remain at their soft starting values. Structural penalties are
ramped up while the structuring temperature schedule advances. Candidate activations are binary
forest features with a fixed split threshold of `0.5`; continuous context features retain trainable
thresholds. Tree leaves remain unrestricted raw-logit contributions. The predictor adds them to
the global bias and applies sigmoid only once to the resulting raw CTR logit.

Run the synthetic experiment with:

```text
SegmentModelTrain.py --config SegmentModelConfig.example.json --output-dir OUTPUT
SegmentModelEvaluate.py --model-dir OUTPUT
SegmentModelExtract.py --model-dir OUTPUT
```

Evaluation constructs `HardSegmentLayer` from the extracted rules and reports soft/hard segment
agreement, activation difference, CTR logloss, ROC-AUC, PR-AUC and permutation-invariant recovery
metrics against synthetic ground truth.
