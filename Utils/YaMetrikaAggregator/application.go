package main

import (
  "context"
  "fmt"
  "os"
  "os/signal"
  "syscall"
  "time"
)

type Application struct {
  config            Config
  logger            *Logger
  pg_client         *PostgresClient
  ya_metrika_client *YaMetrikaClient
  aggregator        Aggregator
  ctx               context.Context
  cancel            context.CancelFunc
}

func (application *Application) close() {
  if application.ya_metrika_client != nil {
    application.ya_metrika_client.close()
  }

  if application.aggregator != nil {
    application.aggregator.close()
  }

  if application.logger != nil {
    application.logger.close()
  }
}

func (application *Application) run() error {
  application.logger.info("Application started")
  go application.update()
  application.wait_for_signal()
  application.logger.info("Application is stopped")
  return nil
}

func (application *Application) update() {
  duration := time.Duration(application.config.ParamsConfig.UpdatePeriodHours) * time.Hour

  for {
    select {
    case <-application.ctx.Done():
      return
    default:
    }

    application.do_work()

    select {
    case <-time.After(duration):
    case <-application.ctx.Done():
      return
    }
  }
}

func (application *Application) do_work() {
  references, err := application.pg_client.fetch()
  if err != nil {
    application.logger.error(
      "failed to read yandex references from postgres",
      "error",
      err)
    return
  }

  if len(references) == 0 {
    application.logger.info("No yandex metrika in postgres")
    return
  } else {
    today := time.Now().Truncate(24 * time.Hour)
    check_days := application.config.ParamsConfig.MetricsWindowDays

    for _, ref := range references {
      for i := check_days - 1; i >= 0; i-- {
        date := today.AddDate(0, 0, -i).Format("2006-01-02")
        application.logger.info(
          "Start handle metrika:",
          "ymref_id",
          ref.ymref_id,
          "metrika_id",
          ref.metrika_id,
          "status",
          ref.token,
          "date=",
          date)

        err = application.ya_metrika_client.handle(
          ref.metrika_id,
          ref.ymref_id,
          ref.token,
          date)
        if err != nil {
          msg := fmt.Sprintf(
            "failed handle metrika: %w",
            err)
          application.logger.error(msg)
          break
        } else {
          application.logger.info("Handling was successful")
        }
      }
    }
  }
}

func (application *Application) wait_for_signal() {
  sig_channel := make(
    chan os.Signal,
    1)
  signal.Notify(
    sig_channel,
    syscall.SIGINT,
    syscall.SIGTERM)

  <-sig_channel

  application.cancel()
}

func write_pid_to_file(file_path string) error {
  file, err := os.OpenFile(
    file_path,
    os.O_WRONLY|os.O_CREATE|os.O_TRUNC,
    0644)
  if err != nil {
    return fmt.Errorf(
      "can't open or create file %q: %w",
      file_path,
      err)
  }
  defer file.Close()

  pid := os.Getpid()
  _, err = fmt.Fprintf(
    file,
    "%d\n",
    pid)
  if err != nil {
    return fmt.Errorf(
      "can't write pid to file %q: %w",
      file_path,
      err)
  }

  return nil
}

func create_application(config_path string) (*Application, error) {
  var config Config
  if err := config.load(config_path); err != nil {
    return nil, err
  }

  logger, err := create_logger(config.LoggingConfig)
  if err != nil {
    return nil, err
  }

  err = write_pid_to_file(config.ParamsConfig.PidPath)
  if err != nil {
    msg := fmt.Sprintf(
      "Can't write pid to file=%s, Reason=%w",
      config.ParamsConfig.PidPath,
      err)
    logger.error(msg)
    return nil, err
  }

  pg_client := create_postgres_client(
    config.PostgresConfig,
    config.ParamsConfig.PostgresTimeout)
  ctx, cancel := context.WithCancel(context.Background())

  http_client := create_http_client(
    config.HttpConfig.ConnectTimeout,
    config.HttpConfig.Limit,
    logger,
    ctx)

  clickhouse_client, err := create_clickhouse_client(
    config.ClickhouseConfig.Host,
    config.ClickhouseConfig.Port,
    config.ClickhouseConfig.User,
    config.ClickhouseConfig.Password,
    config.ClickhouseConfig.Db,
    ctx,
    logger)
  if err != nil {
    return nil, err
  }

  ya_metrika_client := create_ya_metrika_client(
    http_client,
    logger,
    ctx,
    clickhouse_client,
    config.HttpConfig.MaxAttempts)

  return &Application{
    config:            config,
    logger:            logger,
    pg_client:         pg_client,
    ya_metrika_client: ya_metrika_client,
    aggregator:        clickhouse_client,
    ctx:               ctx,
    cancel:            cancel,
  }, nil
}
