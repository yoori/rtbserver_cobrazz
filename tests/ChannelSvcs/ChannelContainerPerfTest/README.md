# ChannelContainerPerfTest

Build and run:

```bash
cmake -S . -B build
cmake --build build --target ChannelContainerPerfTest -j 4
build/bin/ChannelContainerPerfTest \
  --segments /path/to/segments \
  --url 'https://example.com/catalog/item' \
  --iterations 1000000
```

The input directory contains numeric segment directories:

```text
segments/
  123/
    page
    urls
    url_keywords
  456/
    urls
```

Each file contains one positive trigger per line, in the standard trigger parser syntax.
Blank lines are ignored; CRLF is supported. Missing files are allowed. Invalid triggers fail
with the file name and line number. Segment IDs must be positive 32-bit integers; numeric aliases
such as `1` and `01` are rejected as duplicates. Non-numeric directories are ignored.

The test loads all three trigger types through `TriggerParser` and `UpdateContainer`, marks
channels active, and merges them into a `ChannelContainer` with 128 chunks. URL address parsing and
keyword extraction happen once before measurement. The keyword text is passed unnormalized to
`ChannelContainer::match()`; normalization, word splitting, hard phrase generation, deduplication
and matching happen inside each measured call. No page text or search terms are supplied.

The single-threaded loop calls `ChannelContainer::match` exactly `--iterations` times with a fresh
result. Process CPU time (`CLOCK_PROCESS_CPUTIME_ID`) and wall time cover the entire loop, including
keyword preparation and result construction/destruction. Loading triggers, parsing URL addresses,
extracting their keyword text and console output are outside the measured interval.

`--keyword-input raw` (default) uses direct `KeywordInput` -> `PreparedMatchWords` preparation.
`--keyword-input legacy` normalizes the same text, runs the old deferred parser, then calls the
compatibility matching entry point, all within the timed loop. This permits comparison with equal
measurement boundaries. Earlier profiles excluded normalization and splitting and therefore are
not directly comparable with these timings.

Output includes CPU seconds, wall seconds, CPU microseconds per match and the sum of matched
channel counts. Repeating one URL does not reproduce the cache behavior of varied production
traffic. There is no unreported warm-up loop.

URL prefix index regression tests:

```bash
cmake --build build --target UrlPrefixIndexTest -j 4
build/bin/UrlPrefixIndexTest
```

These cover hash collisions, copy-on-write snapshots, shared URL keys and ancestors, deletion,
reinsertion, exact and non-strict matching, dense paths, and matching concurrently with merges.
The prefix index counts URL keys rather than trigger instances. A merge publishes its new chunks
and prefix index together; readers retain their old snapshot. This keeps pruning consistent, but
retains the old tables until the entire merge is ready, increasing peak memory for large updates.

Hard keyword hooks are enabled by default. Use `--keyword-hooks off` for an unfiltered reference
run with the same binary. Both modes include normalization, splitting and candidate generation
in the measured loop.

The production server passes borrowed text, separator characters, normalization mode and optional
segmentor via `KeywordInput`. After acquiring the chunk/hooks snapshot, `ChannelContainer::match()`
prepares the final vector/set directly on the request arena. Canonical text is borrowed; changed
normalization output and accepted noncontiguous hard phrases are stored on the arena. Search word
order is retained in an arena vector, and its precomputed adapters are also added to URL keywords.
The old `MatchWords`/deferred parser path remains for compatibility callers and regression
comparison.

Hooks start at two words and include all hard parts, not only the selected index key. Their snapshot
is published together with chunks during merge. Hash collisions only add candidates; full matching
still checks strings. Raw input can be reused across source updates and is prepared using current
hooks on every call. The old maximum sequence length applies only when hooks are disabled.

`UrlPrefixIndexTest` compares raw preparation against the legacy parser across separators,
normalization modes, hooks, limits, repeated/oversized words and exact-search order. It also checks
borrowing canonical input, raw nonstrict matching, search-to-URL keyword propagation and updates.

`PreparedMatchWords` stores unique candidates in `MonoVector<StringViewHashAdapter>` and keeps a
`MonoUnorderedSet` for deduplication and soft matching. Both are populated before matching starts;
matching iterates only the vector. Candidate entries preserve string views and precomputed hashes.
The nonstrict path expands a separate candidate collection while retaining the original word set.
One 4 KiB stack-backed arena in `ChannelContainer::match()` serves all prepared keyword sets/vectors
and nonstrict candidate/matcher sets; larger requests allocate arena chunks from the heap. The arena
outlives these temporary sets. Input text and the result retain their separate lifetimes. Tests
exercise heap fallback, repeated candidates and consecutive large/small requests. Raw preparation
starts with room for eight candidates and grows as needed. Existing hashes are reused at matcher
boundaries.

URL parsing stores domain suffix candidates and two complete host/path branches (original host and
its www alias), without expanding the path into a list of prefix/postfix strings. Strict matching
visits one branch at a time using string views. It only advances to the next path component when
URL hints report descendants; missing descendants end the branch immediately. Query slashes are
excluded from path traversal, the existing depth limit is retained, and only the original host
carries the exact marker. Prepared branches are independent of the index snapshot and reusable
across updates. Nonstrict matching retains full expansion when passed lazy branches.
