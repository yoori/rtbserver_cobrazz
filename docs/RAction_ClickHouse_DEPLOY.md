# RAction → ClickHouse + Android OSVERSION regex fix

## Why this replaces AdvertiserAction dual-copy (PR #46)

Critical re-check of the pipeline:

1. `RequestLogLoader` sends AdvertiserAction **with `action_id`** via `process_custom_action(AdvExActionInfo)`.
2. `ResearchActionLogger::process_custom_action` already writes **`RAction`** CSV (`UID`, `Action ID`, `URL`/referrer, order…).
3. `process_adv_action` is only used when `action_id` is absent (minimal `AdvActionInfo`) — not the normal `/conv?convid=…` path.

So pixel hits with `convid` already land in **RAction**. Pulling raw `AdvertiserAction` from FE/CM hosts was unnecessary SyncLogs complexity.

This change:

- Side-copies **RAction** from RIM → Predictor `ResearchAction` (unchanged for PredictorMerger) **and** `RActionClickhouse` (CH inbox).
- ClickhouseUploader consumes **only** `RActionClickhouse` and deletes after successful INSERT (does not touch Predictor’s `ResearchAction` backlog).
- Fixes Android OSVERSION regexes (broken double-escaped `\\s`).

## Files

| Path | Change |
|---|---|
| `RequestInfoSvcs/.../RequestOutLogger.cpp` | Comment why `process_adv_action` stays empty |
| `CMS/Plugin/xslt/.../SyncLogs.xsl` | RAction dedicated FeedRouteGroup + side-copy |
| `CMS/Plugin/exec/synclogs_rsync_side_copy.sh` | primary OK / side best-effort |
| `CMS/Plugin/exec/DACSConf.sh` | install helper script |
| `CMS/Plugin/xslt/.../ClickhouseUploader.xsl` | watch `RActionClickhouse` |
| `bin/RActionClickhouseAdapter.py` | RAction CSV → CH |
| `bin/RImpressionStatUploader.py` | `RAction` processor |
| `DACS/.../SyncLogsServer.pm` | mkdir inbox |
| `DACS/.../ClickhouseUploader.pm` | mkdir inbox |
| `docs/sql/2026-08-03_raction_clickhouse.sql` | CH DDL |
| `docs/sql/2026-08-03_fix_android_osversion_regexp.sql` | Android regex fix |
| `bin/validate_raction_clickhouse_pr.py` | local smoke checks (adapter/xsl/regex/side-copy) |

## Deploy (minimal)

1. `clickhouse-client -h click00 --multiquery < docs/sql/2026-08-03_raction_clickhouse.sql`
2. On Postgres `stat`: run `docs/sql/2026-08-03_fix_android_osversion_regexp.sql`  
   Verify: `match_regexp` is exactly `android N([._;]|$)` and contains **no** `\`.
3. Regenerate CMS configs; ensure `synclogs_rsync_side_copy.sh` next to `copy_and_backup.sh`.
4. Restart ClickhouseUploader (adbe00), then SyncLogs on RIM hosts.
5. Check `…/RActionClickhouse` files appear and disappear; `SELECT count() FROM RAction WHERE time > now() - 1 HOUR`.
6. Confirm `…/ResearchAction` still receives files (Predictor path).
7. Optional local checks (no C++ build): `python3 bin/validate_raction_clickhouse_pr.py`

### Android regex check (before COMMIT)

```sql
SELECT p.name, pd.match_regexp,
       position(E'\\' IN pd.match_regexp) AS first_bs
FROM platformdetector pd
JOIN platform p ON p.platform_id = pd.platform_id
WHERE p.name = 'Android 8';
```

After fix: `match_regexp = android 8([._;]|$)`, `first_bs = 0`.

## Known limits

- `URL` in RAction is **page referrer**, not full `/conv?...` query string.
- RAction side-copy group does not re-implement predictor `auxiliaryRef` / backup chain (primary still lands on main predictor `ResearchAction`).
- Historical `ResearchAction` backlog on adbe00 is **not** auto-uploaded (inbox starts empty for new files only). Clean backlog separately if needed.
- Actions **without** `action_id` still do not appear in RAction (by design of RequestLogLoader).

## Rollback

Revert SyncLogs / ClickhouseUploader configs and restart services. Android SQL can be reverted to previous patterns if required (not recommended).
