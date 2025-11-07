package main

import (
  "context"
  "fmt"
  "time"
)

type YaMetrikaDataBatch struct {
  items    []*MetrikaDataItem
  ymref_id uint32
}

type YaMetrikaClient struct {
  http_client   *HttpClient
  logger        *Logger
  ctx           context.Context
  aggregator    Aggregator
  max_attempt   int
  items_channel chan YaMetrikaDataBatch
  stop_channel  chan struct{}
}

func (client *YaMetrikaClient) handle(
  metrika_id uint64,
  ymref_id uint32,
  token string,
  date string) error {

  limit := client.http_client.limit
  offset := 1
  delay := 4 * time.Second

  for {
    i := 1
    for ; i <= client.max_attempt; i++ {
      items, err := client.http_client.fetch(
        metrika_id,
        token,
        date,
        offset)
      if err != nil {
        if api_err, ok := err.(*ApiError); ok {
          if len(api_err.Errors) > 0 && api_err.Errors[0].ErrorType == "invalid_token" {
            return fmt.Errorf(
              "Can't get yandex metrika for metrika_id=%d, token=%s and date=%s. Reason: invalid token[%w]",
              metrika_id,
              token,
              date,
              err)
          }
        } else {
          client.logger.info(
            "Error during fetch, attempt",
            "attempt",
            i,
            "error",
            err)
          delay *= 2
          select {
          case <-time.After(delay):
          case <-client.ctx.Done():
            return fmt.Errorf(
              "Can't get yandex metrika for metrika_id=%d, token=%s and date=%s. Reason: %w",
              metrika_id,
              token,
              date,
              client.ctx.Err())
          }
          continue
        }
      }

      if i > client.max_attempt {
        return fmt.Errorf(
          "Can't get yandex metrika for metrika_id=%d, token=%s and date=%s. Reason: max attempt %d is reached.",
          metrika_id,
          token,
          date,
          client.max_attempt)
      }

      if len(items) != 0 {
        batch := YaMetrikaDataBatch{
          ymref_id: ymref_id,
          items:    items,
        }

        select {
        case client.items_channel <- batch:
        case <-client.ctx.Done():
          return fmt.Errorf(
            "Context cancelled while sending to aggregator channel: %w",
            client.ctx.Err())
        }
      }

      if len(items) < limit {
        message := fmt.Sprintf(
          "Success fetch yandex metrika for metrika_id=%d, token=%s and date=%s",
          metrika_id,
          token,
          date)
        client.logger.info(message)
        return nil
      }

      offset += limit
    }
  }

  return nil
}

func (client *YaMetrikaClient) run() {
  go func() {
    for {
      select {
      case batch_data := <-client.items_channel:
        if err := client.aggregator.send(batch_data.ymref_id, batch_data.items); err != nil {
          client.logger.error(
            "Failed to send batch to aggregator",
            "error",
            err,
            "count",
            len(batch_data.items))
        }
      case <-client.stop_channel:
        return
      case <-client.ctx.Done():
        return
      }
    }
  }()
}

func (client *YaMetrikaClient) close() {
  close(client.stop_channel)
}

func create_ya_metrika_client(
  http_client *HttpClient,
  loger *Logger,
  ctx context.Context,
  aggregator Aggregator,
  max_attempt int) *YaMetrikaClient {

  items_channel := make(chan YaMetrikaDataBatch, 100)
  stop_channel := make(chan struct{})
  client := &YaMetrikaClient{
    http_client:   http_client,
    logger:        loger,
    ctx:           ctx,
    aggregator:    aggregator,
    max_attempt:   max_attempt,
    items_channel: items_channel,
    stop_channel:  stop_channel,
  }

  client.run()

  return client
}
