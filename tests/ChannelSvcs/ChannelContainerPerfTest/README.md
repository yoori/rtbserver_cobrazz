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
channels active, and merges them into a `ChannelContainer` with 128 chunks. URL parsing and URL
keyword extraction happen once before measurement. No page text or search terms are supplied:
page triggers occupy the loaded container but do not match this URL-only request.

The single-threaded loop calls `ChannelContainer::match` exactly `--iterations` times with the
same prepared request and a fresh result. There is no unreported warm-up loop. Process CPU time
(`CLOCK_PROCESS_CPUTIME_ID`) and wall time cover only this loop, including result construction,
destruction and accumulation of the matched-channel count. File I/O, parsing, merging, request
preparation and console output are outside the measured interval.

Output includes CPU seconds, wall seconds, CPU microseconds per match and the sum of matched
channel counts across iterations. Repeating one URL measures repeated access to that request's
working set; it does not reproduce the cache behavior of varied production traffic.
