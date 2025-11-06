package main

import (
  "context"
  "fmt"
  "github.com/jackc/pgx/v5"
  "time"
)

type YandexMetrikaRef struct {
  ymref_id   uint32
  metrika_id uint64
  token      string
}

type PostgresClient struct {
  config  PostgresConfig
  timeout int
}

func create_postgres_client(
  config PostgresConfig,
  timeout int) *PostgresClient {
  return &PostgresClient{
    config:  config,
    timeout: timeout,
  }
}

func (postgres *PostgresClient) connect(ctx context.Context) (*pgx.Conn, error) {
  connection_str := fmt.Sprintf(
    "postgres://%s:%s@%s:%d/%s?sslmode=disable",
    postgres.config.User,
    postgres.config.Password,
    postgres.config.Host,
    postgres.config.Port,
    postgres.config.Db,
  )

  connection, err := pgx.Connect(ctx, connection_str)
  if err != nil {
    return nil, fmt.Errorf("failed to connect to DB: %w", err)
  }

  return connection, nil
}

func (postgres *PostgresClient) fetch() ([]YandexMetrikaRef, error) {
  ctx, cancel := context.WithTimeout(
    context.Background(),
    time.Duration(postgres.timeout)*time.Millisecond)
  defer cancel()

  connection, err := postgres.connect(ctx)
  if err != nil {
    return nil, err
  }
  defer connection.Close(ctx)

  query := `
    SELECT 
      ymref_id, metrika_id, token 
    FROM 
      YandexMetrikaRef 
    WHERE 
      status = 'A'`
  rows, err := connection.Query(
    ctx,
    query)
  if err != nil {
    return nil, fmt.Errorf("failed to execute query: %w", err)
  }
  defer rows.Close()

  var results []YandexMetrikaRef
  for rows.Next() {
    var ref YandexMetrikaRef
    err := rows.Scan(
      &ref.ymref_id,
      &ref.metrika_id,
      &ref.token)
    if err != nil {
      return nil, fmt.Errorf("failed to scan row: %w", err)
    }
    results = append(results, ref)
  }

  if err := rows.Err(); err != nil {
    return nil, fmt.Errorf("row iteration error: %w", err)
  }

  return results, nil
}
