package main

import (
  "context"
  "encoding/json"
  "fmt"
  "io"
  "net"
  "net/http"
  "net/url"
  "strconv"
  "time"
)

type MetrikaDataItem struct {
  date               string
  hour               string
  minute             string
  utm_source         string
  utm_content        string
  utm_term           string
  referer            string
  visits             uint32
  bounces            uint32
  avg_visit_duration float32
}

type ApiError struct {
  Errors []struct {
    ErrorType string `json:"error_type"`
    Message   string `json:"message"`
  } `json:"errors"`
  Code    int    `json:"code"`
  Message string `json:"message"`
}

type DataItem struct {
  Dimensions []json.RawMessage `json:"dimensions"`
  Metrics    []float64         `json:"metrics"`
}

type NameObject struct {
  Name string `json:"name"`
}

type HourObject struct {
  ID   string `json:"id"`
  Name string `json:"name"`
}

type RefererObject struct {
  Name    string `json:"name"`
  Favicon string `json:"favicon"`
}

type HttpClient struct {
  connect_timeout int
  limit           int
  logger          *Logger
  ctx             context.Context
}

func (api_error *ApiError) Error() string {
  return fmt.Sprintf(
    "API error: %s, code: %d",
    api_error.Message,
    api_error.Code)
}

func (client *HttpClient) fetch(
  metrika_id uint64,
  token string,
  date string,
  offset int) ([]*MetrikaDataItem, error) {
  http_client := &http.Client{
    Transport: &http.Transport{
      DialContext: (&net.Dialer{
        Timeout:   time.Duration(client.connect_timeout) * 1000 * time.Millisecond,
        KeepAlive: 30 * time.Second,
      }).DialContext,
    },
    Timeout: 0,
  }

  url_string := "https://api-metrika.yandex.net/stat/v1/data"

  params := url.Values{}
  params.Add("ids", strconv.FormatUint(metrika_id, 10))
  params.Add("metrics", "ym:s:visits,ym:s:bounces,ym:s:avgVisitDurationSeconds")
  params.Add("dimensions", "ym:s:date,ym:s:hour,ym:s:minute,ym:s:UTMSource,ym:s:UTMContent,ym:s:UTMTerm,ym:s:referer")
  params.Add("date1", date)
  params.Add("date2", date)
  params.Add("limit", strconv.Itoa(client.limit))
  params.Add("offset", strconv.Itoa(offset))
  params.Add("sort", "ym:s:date,ym:s:hour,ym:s:minute")

  req, err := http.NewRequestWithContext(client.ctx, "GET", url_string, nil)
  if err != nil {
    return nil, fmt.Errorf("failed to create request: %w", err)
  }

  req.Header.Set("Authorization", fmt.Sprintf("OAuth %s", token))
  req.Header.Set("Content-Type", "application/json")
  req.URL.RawQuery = params.Encode()

  resp, err := http_client.Do(req)
  if err != nil {
    return nil, fmt.Errorf("failed to execute request: %w", err)
  }
  defer resp.Body.Close()

  if resp.StatusCode != http.StatusOK {
    body, _ := io.ReadAll(resp.Body)
    var api_error ApiError
    if err := json.Unmarshal(body, &api_error); err == nil && len(api_error.Errors) > 0 {
      return nil, &api_error
    }

    return nil, fmt.Errorf("api returned status %d: %s", resp.StatusCode, string(body))
  }

  return client.parse_stream_response(resp.Body)
}

func (client *HttpClient) parse_stream_response(body io.Reader) ([]*MetrikaDataItem, error) {

  decoder := json.NewDecoder(body)
  data_items := make([]*MetrikaDataItem, 0, client.limit)

  for {
    select {
    case <-client.ctx.Done():
      return nil, client.ctx.Err()
    default:
      token, err := decoder.Token()
      if err == io.EOF {
        return data_items, nil
      }
      if err != nil {
        return nil, fmt.Errorf("failed to read token: %w", err)
      }

      if key, ok := token.(string); ok && key == "data" {
        t, err := decoder.Token()
        if err != nil {
          return nil, fmt.Errorf("failed to read array opening bracket: %w", err)
        }

        if delim, ok := t.(json.Delim); !ok || delim != '[' {
          return nil, fmt.Errorf("expected array opening bracket, got %v", t)
        }

        for decoder.More() {
          select {
          case <-client.ctx.Done():
            return nil, client.ctx.Err()
          default:
            var data_item DataItem
            if err := decoder.Decode(&data_item); err != nil {
              return nil, fmt.Errorf("failed to decode data item: %w", err)
            }

            parsed_item, err := parse_data_item(
              data_item.Dimensions,
              data_item.Metrics)
            if err != nil {
              return nil, fmt.Errorf("failed to parse data item: %w", err)
            }

            data_items = append(data_items, parsed_item)
          }
        }
      }
    }
  }
}

func parse_data_item(
  dimensions []json.RawMessage,
  metrics []float64) (*MetrikaDataItem, error) {
  if len(dimensions) != 7 {
    return nil, fmt.Errorf("expected 7 dimensions, got %d", len(dimensions))
  }

  if len(metrics) != 3 {
    return nil, fmt.Errorf("expected 3 metrics, got %d", len(metrics))
  }

  item := &MetrikaDataItem{}

  var date_object NameObject
  if err := json.Unmarshal(dimensions[0], &date_object); err != nil {
    return nil, fmt.Errorf("failed to parse date: %w", err)
  }
  item.date = date_object.Name

  var hour_object HourObject
  if err := json.Unmarshal(dimensions[1], &hour_object); err != nil {
    return nil, fmt.Errorf("failed to parse hour: %w", err)
  }
  item.hour = hour_object.ID

  var minute_object NameObject
  if err := json.Unmarshal(dimensions[2], &minute_object); err != nil {
    return nil, fmt.Errorf("failed to parse minute: %w", err)
  }
  item.minute = minute_object.Name

  var utm_source_object NameObject
  if err := json.Unmarshal(dimensions[3], &utm_source_object); err != nil {
    return nil, fmt.Errorf("failed to parse utm_source: %w", err)
  }
  item.utm_source = utm_source_object.Name

  var utm_content_object NameObject
  if err := json.Unmarshal(dimensions[4], &utm_content_object); err != nil {
    return nil, fmt.Errorf("failed to parse utm_content: %w", err)
  }
  item.utm_content = utm_content_object.Name

  var utm_term_obj NameObject
  if err := json.Unmarshal(dimensions[5], &utm_term_obj); err != nil {
    return nil, fmt.Errorf("failed to parse utm_term: %w", err)
  }
  item.utm_term = utm_term_obj.Name

  var referer_object RefererObject
  if err := json.Unmarshal(dimensions[6], &referer_object); err != nil {
    return nil, fmt.Errorf("failed to parse referer: %w", err)
  }
  item.referer = referer_object.Name

  item.visits = uint32(metrics[0])
  item.bounces = uint32(metrics[1])
  item.avg_visit_duration = float32(metrics[2])

  return item, nil
}

func create_http_client(
  connect_timeout int,
  limit int,
  logger *Logger,
  ctx context.Context) *HttpClient {
  return &HttpClient{
    connect_timeout: connect_timeout,
    limit:           limit,
    logger:          logger,
    ctx:             ctx,
  }
}
