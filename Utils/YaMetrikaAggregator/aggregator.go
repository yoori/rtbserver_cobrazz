package main

type Aggregator interface {
  send(ymref_id uint32, items []*MetrikaDataItem) error
  close() error
}
