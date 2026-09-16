# Production CPU-affinity sample

Observed interval: 2026-09-15 13:32:16 UTC — 2026-09-15 13:42:17 UTC.

Observed duration: 600.3 seconds on adfe100–102 and adfe200–201.

`points` is p75 of per-host mean runnable share for a worker in the pool. Runnable share includes CPU runtime and runqueue wait and is capped at 100%.

| Service/pool | Threads/host | CPU % | Wait % | Runnable % | Points |
|---|---:|---:|---:|---:|---:|
| `FCGIRtbServer/bid-request` | 20 | 46.3 | 34.0 | 80.3 | 82 |
| `UserBindServer/rdb-batch` | 16 | 37.3 | 40.9 | 78.2 | 81 |
| `UserBindServer/ub-grpc-p` | 8 | 16.5 | 56.2 | 72.7 | 80 |
| `FCGIRtbServer/fcgi-accept` | 16 | 15.5 | 60.6 | 76.1 | 77 |
| `UserInfoManager/uim-grpc-p` | 10 | 24.9 | 30.8 | 55.7 | 62 |
| `FCGIRtbServer/fast-scheduler` | 10 | 4.3 | 54.4 | 58.7 | 61 |
| `CampaignManager/cm-logging` | 4 | 19.8 | 30.8 | 50.6 | 56 |
| `ChannelServer/cs-grpc-p` | 16 | 33.8 | 15.6 | 49.5 | 54 |
| `FCGIRtbServer/grpc-asio-p` | 10 | 12.8 | 38.6 | 51.4 | 53 |
| `FCGIRtbServer/grpc-common` | 16 | 13.9 | 37.8 | 51.8 | 53 |
| `UserBindServer/fast-scheduler` | 1 | 6.8 | 42.1 | 48.9 | 52 |
| `UserInfoManager/rdb-batch` | 16 | 16.5 | 30.8 | 47.3 | 50 |
| `CampaignManager/cm-grpc-p` | 16 | 17.0 | 17.8 | 34.9 | 41 |
| `FCGIRtbServer/event_engine` | 16 | 3.6 | 36.0 | 39.6 | 41 |
| `CampaignManager/bs-grpc-c` | 1 | 5.8 | 20.3 | 26.0 | 34 |
| `UserInfoManager/fast-scheduler` | 1 | 4.6 | 22.0 | 26.5 | 31 |
| `UserBindServer/event_engine` | 16 | 1.5 | 26.6 | 28.1 | 29 |
| `UserInfoManager/event_engine` | 16 | 1.3 | 17.5 | 18.8 | 22 |
| `CampaignManager/event_engine` | 16 | 2.1 | 17.0 | 19.1 | 21 |
| `UserBindServer/grpc-server` | 16 | 2.2 | 14.2 | 16.4 | 19 |
| `CampaignManager/grpc-server` | 16 | 2.3 | 9.6 | 11.9 | 16 |
| `CampaignManager/task-runner` | 5 | 8.0 | 4.6 | 12.6 | 16 |
| `ChannelServer/event_engine` | 16 | 1.6 | 13.4 | 15.1 | 16 |
| `CampaignManager/bs-grpc` | 4 | 1.1 | 11.2 | 12.3 | 13 |
| `ChannelServer/grpc-server` | 16 | 2.0 | 8.6 | 10.5 | 13 |
| `UserInfoManager/grpc-server` | 16 | 1.6 | 7.5 | 9.1 | 11 |
| `FCGIUserBindServer/bid-request` | 64 | 0.6 | 3.9 | 4.4 | 5 |
| `FCGIRtbServer/jemalloc_bg` | 4 | 1.5 | 0.4 | 1.9 | 2 |
| `FCGIRtbServer/task-runner` | 9 | 0.8 | 1.4 | 2.2 | 2 |
| `CampaignManager/fast-scheduler` | 1 | 0.0 | 0.0 | 0.0 | 1 |
| `CampaignManager/grpc_global` | 1 | 0.0 | 0.1 | 0.1 | 1 |
| `CampaignManager/grpcpp_sync` | 1 | 0.0 | 0.0 | 0.0 | 1 |
| `CampaignManager/http-server` | 4 | 0.0 | 0.0 | 0.0 | 1 |
| `CampaignManager/jemalloc_bg` | 4 | 0.6 | 0.2 | 0.8 | 1 |
| `CampaignManager/lifeguard` | 1 | 0.0 | 0.2 | 0.2 | 1 |
| `ChannelServer/fast-scheduler` | 1 | 0.0 | 0.0 | 0.0 | 1 |
| `ChannelServer/grpc_global` | 1 | 0.0 | 0.1 | 0.1 | 1 |
| `ChannelServer/grpcpp_sync` | 2 | 0.0 | 0.0 | 0.0 | 1 |
| `ChannelServer/http-server` | 4 | 0.0 | 0.0 | 0.0 | 1 |
| `ChannelServer/jemalloc_bg` | 4 | 0.0 | 0.0 | 0.1 | 1 |
| `ChannelServer/lifeguard` | 1 | 0.0 | 0.2 | 0.2 | 1 |
| `ChannelServer/task-runner` | 2 | 0.3 | 0.2 | 0.4 | 1 |
| `FCGIActionServer/bid-request` | 64 | 0.1 | 0.6 | 0.7 | 1 |
| `FCGIAdServer/bid-request` | 128 | 0.0 | 0.0 | 0.0 | 1 |
| `FCGIRtbServer/grpc_global` | 1 | 0.0 | 0.1 | 0.1 | 1 |
| `FCGIRtbServer/http-server` | 4 | 0.0 | 0.0 | 0.0 | 1 |
| `FCGIRtbServer/lifeguard` | 1 | 0.0 | 0.2 | 0.2 | 1 |
| `FCGITrackServer/bid-request` | 64 | 0.1 | 0.7 | 0.8 | 1 |
| `UserBindServer/grpc_global` | 1 | 0.0 | 0.1 | 0.2 | 1 |
| `UserBindServer/http-server` | 4 | 0.0 | 0.0 | 0.0 | 1 |
| `UserBindServer/jemalloc_bg` | 4 | 0.1 | 0.0 | 0.1 | 1 |
| `UserBindServer/lifeguard` | 1 | 0.0 | 0.3 | 0.3 | 1 |
| `UserBindServer/task-runner` | 13 | 0.0 | 0.0 | 0.0 | 1 |
| `UserInfoManager/grpc_global` | 1 | 0.0 | 0.1 | 0.1 | 1 |
| `UserInfoManager/grpcpp_sync` | 1 | 0.0 | 0.0 | 0.0 | 1 |
| `UserInfoManager/http-server` | 4 | 0.0 | 0.0 | 0.0 | 1 |
| `UserInfoManager/jemalloc_bg` | 4 | 0.8 | 0.3 | 1.1 | 1 |
| `UserInfoManager/lifeguard` | 1 | 0.0 | 0.2 | 0.2 | 1 |
| `UserInfoManager/task-runner` | 13 | 0.0 | 0.0 | 0.0 | 1 |

The full per-host breakdown is in `host_pool_stats.csv`, per-thread data is in `thread_stats.csv`, and collection checks are in `collection_integrity.csv`. Raw scheduler snapshots are in `raw_adfe*.tsv`. Linux `comm` truncation aliases are merged by `POOL_ALIASES` in `analyze.py`.
