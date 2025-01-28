#!/bin/bash

conn="$1"

clickhouse-client $conn --multiquery <<EOF

CREATE TABLE RImpression(
  request_id String,
  timestamp SimpleAggregateFunction(any, Nullable(DateTime('UTC'))),
  device SimpleAggregateFunction(any, Nullable(UInt64)),
  ip SimpleAggregateFunction(any, Nullable(String)),
  uid SimpleAggregateFunction(any, Nullable(String)),
  url SimpleAggregateFunction(any, Nullable(String)),
  publisher_id SimpleAggregateFunction(any, Nullable(UInt64)),
  tag_id SimpleAggregateFunction(any, Nullable(UInt64)),
  etag SimpleAggregateFunction(any, Nullable(String)),
  campaign_id SimpleAggregateFunction(any, Nullable(UInt64)),
  ccg_id SimpleAggregateFunction(any, Nullable(UInt64)),
  ccid SimpleAggregateFunction(any, Nullable(UInt64)),
  geo_ch SimpleAggregateFunction(any, Nullable(String)),
  user_ch SimpleAggregateFunction(any, Nullable(String)),
  imp_ch SimpleAggregateFunction(any, Nullable(String)),
  bid_price SimpleAggregateFunction(any, Nullable(Float64)),
  bid_floor SimpleAggregateFunction(any, Nullable(Float64)),
  alg_id SimpleAggregateFunction(any, Nullable(String)),
  size_id SimpleAggregateFunction(any, Nullable(UInt64)),
  colo_id SimpleAggregateFunction(any, Nullable(UInt64)),
  predicted_ctr SimpleAggregateFunction(any, Nullable(Float64)),
  campaign_freq SimpleAggregateFunction(any, Nullable(UInt32)),
  cr_alg_id SimpleAggregateFunction(any, Nullable(String)),
  predicted_cr SimpleAggregateFunction(any, Nullable(Float64)),
  win_price SimpleAggregateFunction(any, Nullable(Float64)),
  viewability SimpleAggregateFunction(any, Nullable(Int32)),
  click_timestamp SimpleAggregateFunction(any, Nullable(DateTime('UTC'))),
  )
  ENGINE = AggregatingMergeTree
  PARTITION BY sipHash64(request_id) % 100
  ORDER BY request_id
  SETTINGS index_granularity = 8192, parts_to_throw_insert = 16000;

EOF
