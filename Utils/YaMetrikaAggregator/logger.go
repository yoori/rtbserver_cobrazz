package main

import (
  "context"
  "fmt"
  "io"
  "log/slog"
  "os"
  "path/filepath"
  "runtime"
  "strings"
)

type CustomHandler struct {
  writer  io.Writer
  options slog.HandlerOptions
}

func (handler *CustomHandler) Handle(
  ctx context.Context,
  record slog.Record) error {

  timestamp := record.Time.Format("2006-01-02 15:04:05")
  level := record.Level.String()

  var source_info string
  var remaining_attrs []string

  record.Attrs(func(attr slog.Attr) bool {
    if attr.Key == "source" {
      source_info = attr.Value.String()
    } else {
      remaining_attrs = append(
        remaining_attrs,
        fmt.Sprintf(
          " %s=%v",
          attr.Key,
          attr.Value))
    }
    return true
  })

  if source_info == "" {
    source_info = "unknown:0"
  }

  message := strings.Join(remaining_attrs, "")
  if source_info == "" {
    source_info = "unknown:0"
  }

  line := fmt.Sprintf(
    "[%s][%s][%s]: %s%s\n",
    timestamp,
    level,
    source_info,
    record.Message,
    message)

  _, err := handler.writer.Write([]byte(line))
  return err
}

func (handler *CustomHandler) WithAttrs(attrs []slog.Attr) slog.Handler {
  return handler
}

func (handler *CustomHandler) WithGroup(name string) slog.Handler {
  return handler
}

func (handler *CustomHandler) Enabled(
  ctx context.Context,
  level slog.Level) bool {
  return level >= handler.options.Level.Level()
}

type Logger struct {
  logger *slog.Logger
  file   *os.File
}

func log_level_from_int(level int) slog.Level {
  switch level {
  case 0:
    return slog.LevelDebug
  case 1:
    return slog.LevelInfo
  case 2:
    return slog.LevelError
  default:
    return slog.LevelInfo
  }
}

func create_logger(config LoggingConfig) (*Logger, error) {
  if err := os.MkdirAll(config.OutLogDir, 0755); err != nil {
    return nil, fmt.Errorf(
      "failed to create log directory %q: %w",
      config.OutLogDir,
      err)
  }

  log_file, err := os.OpenFile(
    filepath.Join(config.OutLogDir, "YaMetrikaAggregator.log"),
    os.O_CREATE|os.O_WRONLY|os.O_APPEND,
    0644)
  if err != nil {
    return nil, fmt.Errorf("failed to open log file: %w", err)
  }

  level := log_level_from_int(config.LogLevel)
  options := slog.HandlerOptions{
    AddSource: true,
    Level:     level,
  }
  handler := &CustomHandler{
    writer:  log_file,
    options: options,
  }
  slogger := slog.New(handler)

  return &Logger{
    logger: slogger,
    file:   log_file,
  }, nil
}

func (logger *Logger) debug(msg string, args ...any) {
  _, file, line, ok := runtime.Caller(1)
  if !ok {
    logger.logger.Debug(msg, args...)
    return
  }

  logger.logger.Debug(
    msg,
    append(
      args,
      "source",
      fmt.Sprintf("%s:%d", file, line))...)
}

func (logger *Logger) info(msg string, args ...any) {
  _, file, line, ok := runtime.Caller(1)
  if !ok {
    logger.logger.Info(msg, args...)
    return
  }

  logger.logger.Info(
    msg,
    append(
      args,
      "source",
      fmt.Sprintf("%s:%d", file, line))...)
}

func (logger *Logger) error(msg string, args ...any) {
  _, file, line, ok := runtime.Caller(1)
  if !ok {
    logger.logger.Error(msg, args...)
    return
  }

  logger.logger.Error(
    msg,
    append(
      args,
      "source",
      fmt.Sprintf("%s:%d", file, line))...)
}

func (logger *Logger) close() error {
  return logger.file.Close()
}
