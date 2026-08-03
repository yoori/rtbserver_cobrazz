# AdvertiserAction → ClickHouse: deploy guide

Goal: stream `CampaignManager/Out/AdvertiserAction` into ClickHouse table `AdvertiserAction` **without** changing RIM / ActionStat / Postgres processing, with low load and no stuck-file backlog.

This path **does not** store the full `/conv?...` URL (still not in the log). It does store `user_id`, `action_id`, referrer page, etc. for audience segments.

## What the patch changes

| File | Role |
|---|---|
| `bin/AdvertiserActionClickhouseAdapter.py` | TSV → CSV for CH |
| `bin/RImpressionStatUploader.py` | processor `AdvertiserAction` + delete after OK |
| `CMS/Plugin/xslt/LogProcessing/ClickhouseUploader.xsl` | watch `…/AdvertiserAction` |
| `CMS/Plugin/xslt/LogProcessing/SyncLogs.xsl` | dedicated FeedRouteGroup for AdvAction; dual rsync |
| `CMS/Plugin/exec/synclogs_rsync_adv_action_ch.sh` | primary rsync must succeed; CH copy `\|\| true` |
| `CMS/Plugin/exec/DACSConf.sh` | install the shell helper next to `copy_and_backup.sh` |
| `DACS/.../SyncLogsServer.pm` | `mkdir` inbox dir |
| `DACS/.../ClickhouseUploader.pm` | `mkdir` inbox on uploader start |
| `docs/sql/2026-08-03_advertiser_action_clickhouse.sql` | CH DDL + 180d TTL |

### Why this does not break current processing

1. **RIM still receives AdvertiserAction** via the same Hash route semantics (`CampaignManager/Out` → `RequestInfoManager/In/AdvertiserAction` + commit files).
2. AdvertiserAction was **moved to its own FeedRouteGroup** so the dual-copy script is **not** on the Request/Impression/Click hot path (avoids bash overhead on high volume).
3. Side-copy to ClickHouse is **best-effort**: failure does not fail primary rsync → SyncLogs still unlinks after RIM delivery as today.
4. No C++ / CM / ActionFrontend changes.
5. Uploader **deletes** files only after successful `INSERT` (same pattern as RImpression). Failures go to `ClickhouseUploader/Error`.

## Prerequisites

- Access to regenerate CMS configs (same flow you use for SyncLogs / ClickhouseUploader).
- Deploy updated `bin/` scripts to hosts that run ClickhouseUploader (**adbe00**).
- `clickhouse-client` on that host (already used for RImpression).
- Ability to run one SQL on **click00**.
- Restart SyncLogs (FE/CM hosts) and ClickhouseUploader (adbe00) after config roll-out.

## Minimal prod sequence

Do **not** reorder: CH table must exist before uploader starts consuming files.

### 1. Merge / build / ship package

Ship at least:

- `bin/AdvertiserActionClickhouseAdapter.py` (executable bit)
- `bin/RImpressionStatUploader.py`
- `CMS/Plugin/exec/synclogs_rsync_adv_action_ch.sh` (via DACSConf → colo etc)
- regenerated configs from updated XSL
- updated DACS `.pm` if you restart services via DACS

Usual path: merge MR → build → install server package on cluster hosts (same as other bin/ + CMS releases).

### 2. Create ClickHouse table (once)

From a host with `clickhouse-client` (e.g. postdb00):

```bash
clickhouse-client -h click00 --multiquery < docs/sql/2026-08-03_advertiser_action_clickhouse.sql
```

Verify:

```bash
clickhouse-client -h click00 --query "EXISTS TABLE default.AdvertiserAction"
clickhouse-client -h click00 --query "SHOW CREATE TABLE default.AdvertiserAction"
```

### 3. Regenerate and roll SyncLogs + ClickhouseUploader configs

Run your normal CMS/DACS config generation so that:

- `$COLO/synclogs_rsync_adv_action_ch.sh` appears next to `copy_and_backup.sh`
- each host `SyncLogsConfig.xml` has a **separate** FeedRouteGroup for AdvertiserAction whose `remote_copy_command` contains `synclogs_rsync_adv_action_ch.sh` and a `rsync://…/logs/AdvertiserAction/` target when predictor host is set
- `ClickhouseUploaderConfig.json` `check_roots` includes  
  `…/log/Predictor/ResearchLogs/AdvertiserAction`

Sanity grep after generation (on generated tree, not prod yet):

```bash
grep -n 'synclogs_rsync_adv_action_ch\|AdvertiserAction' \
  path/to/generated/*/SyncLogsConfig.xml \
  path/to/generated/*/ClickhouseUploaderConfig.json
```

### 4. Ensure inbox directory on adbe00

If SyncLogsServer / ClickhouseUploader will be restarted via DACS, mkdir is automatic.

Otherwise once:

```bash
mkdir -p /opt/foros/server/var/log/Predictor/ResearchLogs/AdvertiserAction
```

(rsync module `logs` must map into `Predictor/ResearchLogs` as today for ResearchImpression.)

### 5. Restart services (short window)

Order:

1. Deploy scripts + configs to hosts.
2. Restart **ClickhouseUploader** on adbe00 (picks new check_roots + adapter).
3. Restart **SyncLogs** on campaign-manager / FE hosts that emit AdvertiserAction (new FeedRouteGroup + helper script).

Prefer your standard DACS/service restart wrappers. Keep the window short; RIM gap is the same class of risk as any SyncLogs restart.

### 6. Verify (10–20 minutes)

On a CM host (e.g. adfe101):

```bash
# primary path still moves files (directory often empty between flushes)
ls -lt /opt/foros/server/var/log/CampaignManager/Out/AdvertiserAction | head
```

On adbe00 (or via `rsync://adbe00:10168/logs/AdvertiserAction/`):

```bash
# files should appear briefly, then disappear after upload
ls -lt /opt/foros/server/var/log/Predictor/ResearchLogs/AdvertiserAction | head
ls -lt /opt/foros/server/var/log/ClickhouseUploader/Error | head
```

ClickHouse:

```bash
clickhouse-client -h click00 --query "
SELECT count(), min(time), max(time)
FROM AdvertiserAction
WHERE time > now() - INTERVAL 1 HOUR"
```

RIM / stats health (unchanged expectations):

- `RequestInfoManager/In/AdvertiserAction` still receives traffic
- no growth of stuck files in `CampaignManager/Out/AdvertiserAction`
- ActionStat / Postgres action aggregates continue as before

### 7. Disk guardrails

- Table TTL = **180 days** (adjust in SQL if needed).
- Uploader deletes inbox files after successful INSERT.
- Watch `ClickhouseUploader/Error` — if non-empty, fix CH/adapter and clear/reprocess; do not leave forever.
- Do **not** enable writing all pixels into `ResearchAction` (already has multi-month backlog).

## Rollback

1. Revert SyncLogs config to previous generation (or remove AdvAction FeedRouteGroup dual-copy and put `AdvertiserAction` back into the shared Hash route as before) → restart SyncLogs.
2. Remove `AdvertiserAction` from ClickhouseUploader `check_roots` / revert uploader → restart ClickhouseUploader.
3. Optional: `DROP TABLE AdvertiserAction` on click00.

RIM processing returns to the pre-patch path; no CM rebuild required for rollback of the dual-copy alone.

## Manual smoke test (optional, staging)

```bash
# adapter only
AdvertiserActionClickhouseAdapter.py /path/to/AdvertiserAction.sample | head

# full insert (staging)
AdvertiserActionClickhouseAdapter.py /path/to/AdvertiserAction.sample \
  | clickhouse-client -h click00 --query="INSERT INTO AdvertiserAction FORMAT CSVWithNames"
```

## Out of scope

- Full HTTP GET / custom query params on `/conv`
- Cleaning historical `ResearchAction` backlog on adbe00
- Audience UI / export tooling on top of CH
