CREATE TABLE YaMetrikaStats (
  ymref_id UInt32,
  time DateTime,
  utm_source LowCardinality(String),
  utm_content LowCardinality(String),
  utm_term String,
  referer String,
  visits UInt32,
  bounces UInt32,
  avg_time Float32
)
ENGINE = ReplacingMergeTree(visits)
ORDER BY (ymref_id, time, utm_source, utm_content, utm_term, referer);