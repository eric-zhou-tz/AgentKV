# AgentKV

## Time-Travel Debugging for AI Agents

AgentKV records and replays AI agent execution so developers can inspect failures, restore state, and debug workflows.

Modern AI agents are difficult to debug because execution is nondeterministic. When an agent fails halfway through a complex task, there's no easy way to inspect what happened, restore state, or replay from a specific point.

AgentKV solves this by treating the LLM as stateless compute and storing execution externally as structured, versioned state.

This makes agent execution:

- Replayable
- Inspectable
- Recoverable
- Forkable
- Traceable

With AgentKV, developers can:

- Rewind to a specific event
- Inspect exactly what the agent saw and did
- Restore execution state before a failure
- Modify prompts, tool outputs, or state
- Replay workflows without repeating every API call

---

## Core Architecture

```text
LLM / Agent Runtime
        |
        v
JSON Request Layer
        |
        v
Agent Session Layer
        |
        v
KV Store Core
        |
        v
WAL + Snapshots + Recovery
```

The LLM does not need to internally remember the entire workflow. Instead, it emits structured operations while AgentKV stores execution state under session- and event-scoped keys.

This creates a durable execution timeline that can be queried, reconstructed, replayed, and debugged.

## Human-Centered Agent Workflows

As AI agents become more autonomous, debugging is no longer only a systems problem — it is also an interaction problem.

Real-world agent workflows involve continuous coordination between:
- users
- agents
- tools
- external systems
- evolving execution state

AgentKV is designed not only for persistence and replay, but also for building inspectable and collaborative human-agent systems.

By making execution state replayable, traceable, and recoverable, AgentKV enables developers to:
- understand how agents make decisions
- inspect failures collaboratively
- experiment with alternative workflows
- analyze multi-agent interactions
- design safer and more controllable AI systems

This bridges systems engineering, agent orchestration, and human-centered AI workflow design.

## Example Timeline

```text

Session: sess_001
event_001  user_message    "Analyze this repository"
event_002  llm_reasoning   plan generated
event_003  tool_call       read README.md
event_004  tool_result     README contents returned
event_005  state_update    goal refined
event_006  tool_call       run tests
event_007  failure         tests failed

A developer should eventually be able to:

1. Open a failed session
2. Inspect the execution timeline
3. Jump to the failing event
4. View prompts, tool calls, outputs, and state
5. Restore execution state before the failure
6. Modify prompts or tool outputs
7. Replay or fork execution from that exact point

This is the core time-travel debugging workflow.

## Current Status

AgentKV currently builds on a working C++17 key-value store with:

* In-memory `SET`, `GET`, and `DELETE`
* Append-only binary write-ahead log persistence
* Snapshot checkpoints
* Startup crash recovery
* GoogleTest coverage
* Benchmark harness for throughput, latency, recovery, and snapshot timing

The project is now evolving from a general-purpose KV store into an agent-aware execution layer.

## Key Highlights

* **C++17 storage core** using `std::unordered_map` for fast in-memory operations
* **Durable write path** using an append-only binary WAL
* **Snapshot recovery model** with WAL byte-offset tracking
* **Structured JSON request layer** for agent-facing operations
* **Session namespacing** for traceable agent workflows
* **Event-scoped state** for step-by-step debugging
* **Replay-oriented design** inspired by event sourcing and VCR-style response caching
* **GoogleTest coverage** for core storage, WAL replay, snapshots, and recovery
* **Benchmark suite** for throughput, latency, recovery, and snapshot workloads

## What AgentKV Stores

AgentKV stores agent execution as structured state, not just arbitrary strings.

A basic action event includes:

```json
{
  "op": "set_action",
  "session_id": "sess_001",
  "event_id": "event_001",
  "value": {
    "action": {
      "type": "tool_call",
      "name": "search_docs",
      "interaction": {
        "query": "latest benchmark results"
      }
    },
    "state": {
      "goal": "summarize project performance",
      "status": "in_progress",
      "context": {
        "active_file": "benchmark.md",
        "current_step": "retrieval"
      }
    },
    "metadata": {
      "model": "gpt-5.5",
      "timestamp": "2026-05-07T00:00:00Z",
      "trace_id": "trace_abc123"
    }
  }
}
```

Internally, AgentKV can map this to a session- and event-namespaced key:

```text
sessions/sess_001/events/event_001/action
```

The value is the structured JSON payload for that event. This keeps the KV interface simple while giving the agent layer enough structure for debugging, playback, and replay.

## Canonical Operation Types

AgentKV begins with a small canonical operation set.

### Basic KV Operations

```json
{
  "op": "set",
  "key": "example:key",
  "value": "example value"
}
```

```json
{
  "op": "get",
  "key": "example:key"
}
```

```json
{
  "op": "delete",
  "key": "example:key"
}
```

### Agent Action Operation

```json
{
  "op": "set_action",
  "session_id": "sess_001",
  "event_id": "event_001",
  "value": {
    "action": {
      "type": "llm_message",
      "name": "draft_plan",
      "interaction": {}
    },
    "state": {
      "goal": "complete workflow",
      "status": "in_progress",
      "context": {}
    },
    "metadata": {}
  }
}
```

The request layer validates that the JSON is well-formed, identifies the operation, enforces the expected contract for that operation, and dispatches it into the session layer or raw KV layer.

## Debugging Model

AgentKV is designed around a timeline-first debugging workflow.

```text
Session: sess_001

event_001  user_message      "Analyze this repo"
event_002  llm_reasoning     plan generated
event_003  tool_call         read README.md
event_004  tool_result       README contents returned
event_005  state_update      goal refined
event_006  tool_call         run tests
event_007  failure           tests failed
```

A developer should eventually be able to:

1. Open a failed session
2. Inspect the event timeline
3. Jump to the failing event
4. View the action, state, metadata, inputs, and outputs
5. Restore state from before the failure
6. Modify a prompt, tool result, or state patch
7. Replay or fork execution from that exact event

This is the core time-travel debugging loop.

## Persistence Model

AgentKV keeps the active dataset in memory and uses disk for durability and recovery.

1. Mutating commands append a WAL record and flush it before updating memory.
2. `SET` records store an opcode, key length, key bytes, value length, and value bytes.
3. `DELETE` records store an opcode, key length, and key bytes.
4. Snapshots write the full in-memory map to a temporary file, then atomically replace the committed snapshot file.
5. Each snapshot stores the WAL byte offset it covers.
6. Startup recovery loads `kv_store.snapshot` when present, then replays `kv_store.wal` from the saved offset.
7. If no snapshot exists, recovery falls back to WAL replay from offset zero.

Replay safely handles malformed bounded records, incomplete trailing records, and corrupted lengths without unbounded allocations.

## Benchmarks

The repository includes a benchmark executable under `bench/`. It measures the persisted code path: `KVStore` with WAL and snapshot persistence.

```bash
make benchmark
./benchmark
./benchmark 100000
```

The checked-in baseline report is in [`benchmark.md`](benchmark.md). Treat these numbers as local baseline measurements, not universal performance claims. Rerun the suite on the target machine before publishing new results.

| Workload |             Operations |           Throughput | Notes                                    |
| -------- | ---------------------: | -------------------: | ---------------------------------------- |
| Write    |                 20,000 |   223,395.57 ops/sec | Persisted `SET` path with WAL flushes    |
| Read     |                 20,000 | 4,470,689.38 ops/sec | In-memory successful `GET` after preload |
| Mixed    |                 20,000 |   902,398.15 ops/sec | Deterministic 70/30 read/write workload  |
| Recovery | 20,000 base + 999 tail |              8.38 ms | Snapshot load plus WAL tail replay       |
| Snapshot |         20,000 records |              4.21 ms | Explicit full-state snapshot             |

Benchmark conditions to record for future runs:

| Field                  | Value |
| ---------------------- | ----- |
| Machine / CPU / memory | TBD   |
| Operating system       | TBD   |
| Compiler               | TBD   |
| Build mode and flags   | TBD   |
| Storage medium         | TBD   |
| Git commit             | TBD   |

## Testing

Tests are organized by behavior rather than implementation detail:

* Core store tests for `SET`, `GET`, `DELETE`, overwrites, missing keys, and large values
* WAL tests for replay order, malformed records, partial trailing records, and replay from offsets
* Snapshot tests for save/load, legacy snapshot loading, corruption handling, and bounded field sizes
* Recovery integration tests for WAL-only recovery, snapshot-only recovery, snapshot plus WAL tail recovery, and persistence reset behavior
* Stress tests for deterministic mixed workloads, hot-key overwrites, many distinct keys, large values, and large snapshot-plus-WAL recovery

GoogleTest is required for the test targets. The Makefile can use a vendored copy, a system install, or `GTEST_ROOT`.

```bash
./scripts/bootstrap_gtest.sh
make test
make test_verbose
make test_stress
```

## Quick Start

```bash
git clone <repo-url>
cd AgentKV
make
./bin/kv_store
```

Example CLI session:

```text
kv-store> SET language cpp
OK
kv-store> GET language
cpp
kv-store> DELETE language
1
kv-store> EXIT
Bye
```

Useful targets:

```bash
make run
make test
make test_stress
make benchmark
make run_benchmark
make clean
```

Docker is also supported:

```bash
docker build -t agentkv .
docker run --rm -it agentkv
```

## JSON Mode

AgentKV is moving toward JSON-first agent communication.

A simple invocation may look like:

```bash
echo '{"op":"set","key":"demo","value":"hello"}' | ./bin/kv_store --json
```

A future agent action may look like:

```bash
echo '{"op":"set_action","session_id":"sess_001","event_id":"event_001","value":{"action":{"type":"tool_call","name":"search","interaction":{"query":"AgentKV"}},"state":{"goal":"debug agent run","status":"in_progress","context":{}},"metadata":{"trace_id":"trace_001"}}}' | ./bin/kv_store --json
```

The JSON layer should remain intentionally small at first: validate the operation, enforce the canonical schema, and dispatch into the existing storage/session pipeline.

## Project Structure

```text
src/store/          Core in-memory KV store and persistence integration
src/persistence/    WAL, snapshot, and binary I/O implementations
src/parser/         CLI and JSON request parsing
src/command/        Command validation and dispatch
src/agent/          Session-aware agent execution layer
src/server/         Interactive CLI loop and request handling
include/            Public headers for store, persistence, parser, command, agent, and server
tests/              GoogleTest unit, integration, stress, and helper code
bench/              Single-threaded benchmark harness and workloads
scripts/            Build, run, and GoogleTest bootstrap helpers
benchmark.md        Captured baseline methodology and results
DESIGN.md           Engineer-facing storage and recovery design notes
agent.md            Agent execution model and JSON contract notes
```

## Design Decisions

* Keep the storage core small, fast, and testable.
* Preserve the simple KV interface while layering agent semantics above it.
* Use session- and event-namespaced keys for traceability.
* Store structured JSON values at the agent layer instead of forcing the KV core to understand agent-specific schemas.
* Append WAL records before memory mutation so successful writes have durable replay records.
* Use length-prefixed binary records for compact WAL and snapshot formats.
* Bound record and field sizes during replay to avoid unsafe allocations from corrupted files.
* Store the covered WAL offset in each snapshot so recovery can replay only the post-snapshot tail.
* Save snapshots with a temp-file-plus-rename flow so failed snapshot writes do not replace the last complete snapshot.
* Treat deterministic replay as a product-level goal, not just a storage feature.

## Roadmap

### Completed

* [x] `0.1.0` KV Store Core
* [x] `0.2.0` Basic Persistence: WAL + snapshot + recovery

### Current Focus

* [ ] `0.3.0` JSON Request Layer
  Normalize OpenAI/MCP/custom JSON into canonical AgentKV tool calls.

* [ ] `0.4.0` Toy Agent Connection
  Connect a minimal toy agent that can call `set`, `get`, and `delete` through the request layer.

### Agent Execution Layer

* [ ] `0.5.0` Sessions
  `start_session`, `end_session`, `get_session`

* [ ] `0.5.1` Step Logging
  `log_step`, `get_steps`, `get_recent_steps`

* [ ] `0.5.2` Basic Memory
  `save_memory`, `update_memory`, `delete_memory`, `get_memory`, `list_memory`, `search_memory`, scopes

* [ ] `0.5.3` State
  `set_state`, `get_state`, `patch_state`, `diff_state`

* [ ] `0.5.4` Context
  `get_context = recent steps + state + relevant memories`

### Debugging and Replay

* [ ] `0.6.0` Checkpoints
  `create_checkpoint`, `get_checkpoint`, `restore_checkpoint`

* [ ] `0.7.0` Playback
  Reconstruct the execution timeline of a session.

* [ ] `0.7.1` Deterministic Replay
  Replay from logged inputs, outputs, and cached tool/model responses.

* [ ] `0.7.2` Developer-Friendly Audit Log
  Make the timeline readable, searchable, and inspectable.

### Validation and Real-World Testing

* [ ] `0.8.0` Verification and Evaluation Tests
  Validate session reconstruction, replay behavior, and schema contracts.

* [ ] `0.9.0` Real-World Agent Testing
  Test AgentKV against real agent workflows and failure cases.

### Longer-Term Systems Roadmap

* [ ] `2.0.0` Performance + Storage Engine
  SSTables, compaction, segmented WAL, caching, and LSM-style evolution.

* [ ] `3.0.0` Concurrency
  Multi-threaded request handling and safe concurrent access.

* [ ] `4.0.0` Networking
  TCP server, request protocol, and multi-client support.

* [ ] `5.0.0` Replication + Fault Tolerance
  Leader/follower replication, log shipping, and Raft-inspired durability.

* [ ] `6.0.0` Distributed Agent State
  Distributed execution-state storage for larger multi-agent systems.

## Competitive Positioning

AgentKV is not trying to be another agent framework.

Frameworks help define and run workflows. Observability tools help inspect traces after a run. Vector databases help retrieve semantic memory. AgentKV targets a lower-level systems problem: durable, replayable, forkable execution state.

The goal is to make agent debugging feel less like guessing and more like using a debugger, Git history, and event log at the same time.

## Demo Ideas

Suggested demo flow:

1. Start an agent session.
2. Log several agent steps.
3. Trigger a tool failure.
4. Inspect the session timeline.
5. Restore the state before the failure.
6. Patch the tool output or prompt.
7. Replay from the failed event.

Demo GIF placeholder: add a terminal or UI demo showing a failed agent run, event inspection, checkpoint restore, and replay.

## License

TBD.

Recommended default for open-source traction: MIT or Apache-2.0.

## Status Note

AgentKV is early. The storage foundation is working; the agent-aware JSON/session/replay layer is under active development. The near-term goal is not to support every agent framework immediately. The goal is to prove the core debugging loop with a minimal, durable, inspectable execution model.
