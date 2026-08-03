-- ClickHouse DDL for pixel /conv event stream (AdvertiserAction logs).
-- Apply on click00 (or the host from ClickhouseUploader clickhouse_conn).

CREATE TABLE IF NOT EXISTS default.AdvertiserAction
(
    `time` DateTime('UTC'),
    `user_id` String,
    `request_id` String,
    `action_id` UInt64,
    `device_channel_id` UInt64,
    `action_request_id` String,
    `ccg_ids` String,
    `referrer` String,
    `order_id` String,
    `ip` String,
    `cur_value` Float64
)
ENGINE = MergeTree
PARTITION BY toYYYYMM(time)
ORDER BY (action_id, time, user_id)
TTL time + toIntervalDay(180)
SETTINGS index_granularity = 8192;
