CREATE TABLE urls
(
    url String,
    indexed_date DateTime        -- indexation DateTime
)
ENGINE = ReplacingMergeTree()
ORDER BY url;