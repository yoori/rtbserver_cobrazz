-- ClickHouse table for research RAction (pixel / custom actions with action_id).
-- Source: RequestInfoManager ResearchAction → side-copy inbox RActionClickhouse.
-- Columns mirror ResearchActionTraits::csv_header().

CREATE TABLE IF NOT EXISTS default.RAction
(
    `time` DateTime('UTC'),
    `device_channel_id` UInt64,
    `ip` String,
    `user_id` String,
    `referrer` String,
    `action_id` UInt64,
    `order_id` String,
    `cur_value` Float64
)
ENGINE = MergeTree
PARTITION BY toYYYYMM(time)
ORDER BY (action_id, time, user_id)
TTL time + toIntervalDay(180)
SETTINGS index_granularity = 8192;
