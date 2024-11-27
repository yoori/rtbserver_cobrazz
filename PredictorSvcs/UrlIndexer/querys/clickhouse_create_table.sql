CREATE TABLE IF NOT EXISTS urls
(
    url String,
    indexed_date DateTime        -- indexation DateTime
)
ENGINE = ReplacingMergeTree()
ORDER BY url;