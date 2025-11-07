package main

import (
  "fmt"
  "os"
)

func main() {
  if len(os.Args) != 2 {
    fmt.Fprintln(
      os.Stderr,
      "Error: config.xml path is required in command line")
    os.Exit(1)
  }

  config_path := os.Args[1]

  application, err := create_application(config_path)
  if err != nil {
    fmt.Fprintf(
      os.Stderr,
      "Application initialization error: %v\n",
      err)
    os.Exit(1)
  }
  defer application.close()

  if err := application.run(); err != nil {
    fmt.Fprintf(
      os.Stderr,
      "Application is failed: %v\n",
      err)
    os.Exit(1)
  }
}
