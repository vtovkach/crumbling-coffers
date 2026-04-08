# Supervisor Tests

Set target IP in `test-config.h` → `#define IP_ADDRESS "127.0.0.1"`

## Orchestrator
| Command | Description |
|---|---|
| `make test-orchestrator` | Launches supervisor, runs tests |
| `make test-orchestrator-remote` | Runs tests against existing supervisor |

## Matchmaker
| Command | Description |
|---|---|
| `make test-matchmaker` | Launches supervisor, runs tests |
| `make test-matchmaker-remote` | Runs tests against existing supervisor |

`./bin/test_matchmaking [num_clients [ip]]` — defaults: 10 clients, `IP_ADDRESS`
