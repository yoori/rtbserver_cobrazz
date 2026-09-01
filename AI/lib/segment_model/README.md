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

The candidate initialization modes are `random_single_seed`, `symmetric_with_noise` and
`random_multi_seed`; the materialized synthetic-data path also retains `frequency`. Background URL
logits, selected seed logits and their independent noise are configurable. Single-seed mode assigns
one shuffled observed URL to each candidate, symmetric mode does not preselect URLs, and multi-seed
mode independently samples `initial_urls_per_candidate` URLs for every candidate.

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

The global bias starts at the logit of the CTR in the training partition. Leaf contributions and
feature-selection logits start with configurable small zero-centered Gaussian noise; no forest
feature is preselected. A window or visit-threshold choice with one configured value is represented
as a constant gate and does not allocate a trainable categorical logit.

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

All candidates use the same URL sparsity coefficient.

Optional fixed candidate opening inserts a non-trainable `CandidateMaskLayer` between segment
activations and the forest. The same mask removes closed candidates from forest selector softmax,
and auxiliary losses only see open candidates. Closed candidate parameters are restored after each
optimizer step, including protection from weight decay. The scheduler can keep previous candidates
at full effective learning rate or reduce their actual parameter update while the latest candidate
uses the base rate. URL temperature stays above a configured floor until every candidate is open;
an optional final joint phase anneals from that floor while all candidate updates use one reduced
learning-rate multiplier. Existing production channels and context features remain available to the
forest independently of the candidate mask.

Candidate duplicate regularization is separate from generic structural regularization. It uses a
margin over soft URL Jaccard and a margin over absolute centered activation correlation, so partial
URL overlap is not penalized. Its epoch-based delay and linear ramp are configurable. Training can
sample candidate pairs with `duplicate_pairs`; zero evaluates all pairs. Evaluation reports exact
pairwise similarities, thresholds and the most similar pairs with top URLs and soft forest feature
importance. Optional candidate reseeding is disabled by default and only runs after the duplicate
regularization ramp has completed. Best-checkpoint selection and early-stopping patience also start
after that ramp, so restoring the best CTR checkpoint cannot silently restore an early duplicated
candidate state.

Run the synthetic experiment with:

```text
SegmentModelTrain.py --config SegmentModelConfig.example.json --output-dir OUTPUT
SegmentModelEvaluate.py --model-dir OUTPUT
SegmentModelExtract.py --model-dir OUTPUT
```

Evaluation constructs `HardSegmentLayer` from the extracted rules and reports soft/hard segment
agreement, activation difference, CTR logloss, ROC-AUC, PR-AUC and permutation-invariant recovery
metrics against synthetic ground truth.

For an atomically published model that is visible to `SegmentModelViewer`, use a repository root:

```text
SegmentModelTrain.py \
  --config SegmentModelConfig.example.json \
  --repository-root "$workspace_root/log/SegmentGenerator/Models"
```

Training starts in `~YYYYMMDD.HHMMSS`, updates `traits.json` with progress and renames the complete
directory without the `~` prefix. The viewer reads JSON artifacts only; it never imports Torch or
loads `checkpoint.pt`.
