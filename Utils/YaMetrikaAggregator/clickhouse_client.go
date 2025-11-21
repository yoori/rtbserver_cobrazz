package main

import (
  "context"
  "fmt"
  "github.com/ClickHouse/clickhouse-go/v2"
  "strconv"
  "time"
)

type ClickHouseClient struct {
  conn   clickhouse.Conn
  logger *Logger
  ctx    context.Context
}

func (client *ClickHouseClient) send(
  ymref_id uint32,
  items []*MetrikaDataItem) error {

  if len(items) == 0 {
    client.logger.info(
      "No items to send to ClickHouse",
      "total_count",
      0)
    return nil
  }

  batch_size := 500
  query := `
    INSERT INTO 
      YaMetrikaStats (
        ymref_id,
        time,
        utm_source,
        utm_content,
        utm_term,
        referer,
        visits,
        bounces,
        avg_time) 
    VALUES 
      (?, ?, ?, ?, ?, ?, ?, ?, ?)`

  for i := 0; i < len(items); i += batch_size {
    end := i + batch_size
    if end > len(items) {
      end = len(items)
    }
    batch_items := items[i:end]

    err := client.send_batch(
      ymref_id,
      batch_items,
      query)
    if err != nil {
      return fmt.Errorf(
        "failed to send batch starting at index %d: %w",
        i,
        err)
    }
  }

  return nil
}

func (client *ClickHouseClient) send_batch(
  ymref_id uint32,
  batch_items []*MetrikaDataItem,
  query string) error {

  max_retries := 5
  base_delay := 1 * time.Second

  var err error
  for attempt := 1; attempt <= max_retries; attempt++ {
    err = client.execute_single_batch(
      ymref_id,
      batch_items,
      query)
    if err == nil {
      return nil
    }

    if attempt < max_retries {
      delay := base_delay * time.Duration(1<<(attempt-1))
      select {
      case <-time.After(delay):
      case <-client.ctx.Done():
        return fmt.Errorf(
          "context cancelled while retrying batch send: %w",
          client.ctx.Err())
      }
    }
  }

  return fmt.Errorf(
    "failed to send batch after %d attempts: %w",
    max_retries,
    err)
}

func (client *ClickHouseClient) execute_single_batch(
  ymref_id uint32,
  batch_items []*MetrikaDataItem,
  query string) error {

  batch, err := client.conn.PrepareBatch(
    client.ctx,
    query)
  if err != nil {
    return fmt.Errorf(
      "failed to prepare batch: %w",
      err)
  }

  for _, item := range batch_items {
    hour_int, err := strconv.Atoi(item.hour)
    if err != nil || hour_int < 0 || hour_int > 23 {
      client.logger.error(
        "Invalid hour value",
        "value",
        item.hour,
        "item_date",
        item.date)
      continue
    }

    minute_int, err := strconv.Atoi(item.minute)
    if err != nil || minute_int < 0 || minute_int > 59 {
      client.logger.error(
        "Invalid minute value",
        "value",
        item.minute,
        "item_date",
        item.date,
        "item_hour",
        item.hour)
      continue
    }

    date_time_str := fmt.Sprintf(
      "%s %02d:%02d:00",
      item.date,
      hour_int,
      minute_int)

    parsed_time, err := time.Parse(
      "2006-01-02 15:04:05",
      date_time_str)
    if err != nil {
      client.logger.error(
        "Failed to parse time",
        "raw_date",
        item.date,
        "raw_hour",
        item.hour,
        "raw_minute",
        item.minute,
        "formatted_str",
        date_time_str,
        "error",
        err)
      continue
    }

    err = batch.Append(
      ymref_id,
      parsed_time,
      item.utm_source,
      item.utm_content,
      item.utm_term,
      item.referer,
      item.visits,
      item.bounces,
      item.avg_visit_duration)
    if err != nil {
      return fmt.Errorf(
        "failed to append item to batch: %w",
        err)
    }
  }

  if err := batch.Send(); err != nil {
    return fmt.Errorf(
      "failed to send batch: %w",
      err)
  }

  return nil
}

func (client *ClickHouseClient) close() error {
  if client.conn != nil {
    return client.conn.Close()
  }

  return nil
}

func create_clickhouse_client(
  host string,
  port int,
  user string,
  password string,
  db string,
  ctx context.Context,
  logger *Logger) (*ClickHouseClient, error) {
  conn, err := clickhouse.Open(&clickhouse.Options{
    Addr: []string{fmt.Sprintf("%s:%d", host, port)},
    Auth: clickhouse.Auth{
      Database: db,
      Username: user,
      Password: password,
    },
    Settings: clickhouse.Settings{
      "max_execution_time": 60,
    },
    DialTimeout:      10 * time.Second,
    MaxOpenConns:     3,
    MaxIdleConns:     3,
    ConnMaxLifetime:  time.Duration(10) * time.Minute,
    ConnOpenStrategy: clickhouse.ConnOpenInOrder,
    BlockBufferSize:  10,
  })
  if err != nil {
    msg := fmt.Sprintf(
      "failed to connect to ClickHouse: %s",
      err.Error())
    logger.error(msg)
    return nil, fmt.Errorf(msg)
  }

  err = conn.Ping(ctx)
  if err != nil {
    msg := fmt.Sprintf(
      "failed to connect to ClickHouse: %s",
      err.Error())
    logger.error(msg)
    return nil, fmt.Errorf(msg)
  }

  return &ClickHouseClient{
    conn:   conn,
    logger: logger,
    ctx:    ctx,
  }, nil
}
