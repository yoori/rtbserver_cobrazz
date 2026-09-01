# ClickHouse and ExpressionMatcher scenario

This compose project checks the full `RImpression -> HTTP profile -> segment model` path. The
scenario JSON is mounted into both the ClickHouse seeder and the ExpressionMatcher mock, so profile
selection and click generation use the same deterministic rule.

Run the URL-union experiment on `nvidia01` from this directory:

```text
docker compose up --abort-on-container-exit --exit-code-from trainer
```

The default `UnionScenario.json` gives each positive-cohort user either `a.com` or `b.com`, never
both. Use the single-candidate model to force one production rule to describe their union:

```text
MODEL_CONFIG_FILE=SingleCandidateModelConfig.json docker compose up \
  --abort-on-container-exit --exit-code-from trainer
```

Run the original perfectly correlated profile experiment with:

```text
SCENARIO_FILE=CorrelatedScenario.json docker compose up \
  --abort-on-container-exit --exit-code-from trainer
```

Results are written to `output/`. In particular, `segments.json` contains extracted hard rules,
while `metrics.json` contains separate actual and predicted CTR values for `even-positive` and
`odd-zero`, including their individual profile variants. `scenario.json` records the normalized
scenario used by the run. `url-bucket-dictionary.json` maps the model's fixed CRC32 buckets back to
all URLs observed during training.

Training starts as soon as the first prepared batch reaches the bounded ready queue. During that
first pass each worker atomically stores its prepared batch under `output/batch-cache`; later
epochs read the cache instead of repeating HTTP profile requests. The full dataset is never
materialized in RAM before GPU training starts.

The model has one million URL hash buckets by default, but prepared batches contain only the
buckets observed in the current vocabulary. The mock implements ExpressionMatcher's production
POST contract and returns daily navigation counts no later than the requested impression date.

The mock runs eight pre-fork HTTP processes. Together with eight batch workers and 32 profile
threads per worker this prevents the single Python mock process from becoming the only source
bottleneck during the million-request first pass.

The scenario format supports any number of cohorts and profile variants. A navigation entry lists
its ages relative to the impression timestamp:

```json
{"url": "a.com", "ages_seconds": [30, 60, 120]}
```

For a cohort with multiple variants, `click_every_n_per_variant` is evaluated using the ordinal
inside each variant. Therefore all variants receive exactly the same configured CTR.
