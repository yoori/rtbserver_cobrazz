package main

import (
  "encoding/xml"
  "fmt"
  "os"
  "regexp"
)

type Config struct {
  ClickhouseConfig ClickhouseConfig `xml:"ClickhouseConfig"`
  PostgresConfig   PostgresConfig   `xml:"PostgresConfig"`
  HttpConfig       HttpConfig       `xml:"HttpConfig"`
  LoggingConfig    LoggingConfig    `xml:"LoggingConfig"`
  ParamsConfig     ParamsConfig     `xml:"ParamsConfig"`
}

type ClickhouseConfig struct {
  Host     string `xml:"host,attr"`
  Port     int    `xml:"port,attr"`
  Db       string `xml:"db,attr"`
  User     string `xml:"user,attr"`
  Password string `xml:"password,attr"`
}

type PostgresConfig struct {
  Host     string `xml:"host,attr"`
  Port     int    `xml:"port,attr"`
  Db       string `xml:"db,attr"`
  User     string `xml:"user,attr"`
  Password string `xml:"password,attr"`
}

type HttpConfig struct {
  ConnectTimeout int `xml:"connect_timeout,attr"`
  Limit          int `xml:"limit,attr"`
  MaxAttempts    int `xml:"max_attempts,attr"`
}

type LoggingConfig struct {
  LogLevel  int    `xml:"log_level,attr"`
  OutLogDir string `xml:"out_log_dir,attr"`
}

type ParamsConfig struct {
  MetricsWindowDays int    `xml:"metrics_window_days,attr"`
  UpdatePeriodHours int    `xml:"update_period_hours,attr"`
  PostgresTimeout   int    `xml:"postgres_timeout,attr"`
  PidPath           string `xml:"pid_path,attr"`
}

func strip_xml_namespaces(raw []byte) []byte {
  s := string(raw)
  s = regexp.MustCompile(`\s*xmlns:[a-zA-Z0-9_-]+="[^"]*"`).ReplaceAllString(s, "")
  s = regexp.MustCompile(`\s*xmlns="[^"]*"`).ReplaceAllString(s, "")
  s = regexp.MustCompile(`<(/?)([a-zA-Z0-9_-]+:)([a-zA-Z0-9_-]+)`).ReplaceAllString(s, "<$1$3")
  return []byte(s)
}

func (c *Config) load(filename string) error {
  data, err := os.ReadFile(filename)
  if err != nil {
    return fmt.Errorf("failed to read file %q: %w", filename, err)
  }

  strip_data := strip_xml_namespaces(data)
  var wrapper struct {
    Config Config `xml:"YaMetrikaAggregatorConfig"`
  }

  if err := xml.Unmarshal(strip_data, &wrapper); err != nil {
    return fmt.Errorf("XML parsing error from %q: %w", filename, err)
  }

  *c = wrapper.Config
  return nil
}
