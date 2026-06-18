# ScholarDB

A real-time student record system. Add, update, delete, search, and sort student records — changes appear live across every connected client the moment they happen.

---

## Quick Start

**Step 1 — Build**
```bash
make build
```

**Step 2 — Start the server** (keep this terminal open)
```bash
make run-server
```

**Step 3 — Connect a client** (open a new terminal)
```bash
make run-client
```

**Step 4 — Open the browser UI** (optional, works alongside the CLI client)
```bash
make open-browser
```

**Run tests**
```bash
make test
```

> Dependencies (IXWebSocket, Catch2) download automatically on first `make build`. No manual installation needed.

---

## How It Works

There are three pieces:

- **Server** — owns all the data. Loads `students.csv` at startup. Every change is saved back to the file and broadcast to all connected clients.
- **CLI Client** — a terminal menu to list, search, add, update, delete, and sort students.
- **Browser UI** — the same operations in a web page. Open multiple tabs — they all update live.

Any change made from the CLI client immediately appears in the browser, and vice versa.

---

## Languages & Libraries

| | |
|---|---|
| C++17 | Server and CLI client |
| HTML / CSS / JavaScript | Browser UI — no frameworks, single file |
| [IXWebSocket v11.4.5](https://github.com/machinezone/IXWebSocket) | WebSocket library |
| [nlohmann/json v3.11.3](https://github.com/nlohmann/json) | JSON parsing |
| [Catch2 v3.5.2](https://github.com/catchorg/Catch2) | Unit testing |

---

## Bonus Features

- **Live multi-client broadcast** — every mutation is instantly pushed to all connected clients (browser tabs + CLI)
- **Read-only query isolation** — LIST / SEARCH / SORT reply only to the requesting client, never broadcast
- **Thread-safe concurrent access** — `std::shared_mutex` in the repository; multiple clients can read simultaneously
- **Duplicate ID rejection** — creating a student with an existing ID returns an error immediately
- **Case-insensitive search** — searching "alice" matches "Alice Johnson"
- **Non-destructive sort** — sorting returns a copy; the original insertion order is never changed
- **RAII performance timers** — five operations instrumented; stats print automatically when the server stops
- **Structured logging** — every connect, disconnect, CRUD operation, and error is timestamped to stdout
- **Unit tests** — 22 tests / 79 assertions (CSV round-trip, validator, repository CRUD, concurrency)

---

## Performance

Measured on Apple M-series, loopback, 6-student dataset. Press Ctrl-C on the server to see live numbers.

| Operation | Avg (µs) | Notes |
|---|---|---|
| CSV load | 361 | Once at startup |
| Sort | 13 | Returns a sorted copy |
| WebSocket transmit | 25 | Per client, per send |
| CSV save | 613 | Full file rewrite on every mutation |
| Broadcast | 27 | Full loop over all connected clients |

`csv_save` is the slowest operation because the file is completely rewritten on every change. All others are well under 1 ms at this scale.

---

## Known Limitations

- **Commas in names will break CSV parsing.** A name like `Smith, Jr.` contains a comma that the parser treats as a field separator. Not a problem with normal names.
- **No reconnect logic.** If the server restarts, refresh the browser or re-run the CLI client.
- **No TLS.** Runs on plain `ws://` — fine for local use, not for production over a network.
- **Performance numbers are from a 6-row dataset.** They are illustrative; run with a larger CSV for meaningful benchmarks.
