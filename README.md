# Social Media Feed Sorter — C++ core, Express API, live browser demo

This is a real REST API backed by a compiled multi-threaded C++ sorting
engine — not a JavaScript demo pretending to sort posts. The browser calls
Express, which spawns the actual compiled binary as a subprocess and returns
genuinely computed results.

```
Browser (public/) --POST /api/sort--> Express (server.js) --spawns--> feed_sorter_cli (C++, -pthread)
                                                                              |
                                                                     real parallel merge-sort
                                                                     (see backend/src/FeedManager.cpp)
```

**Live demo:** _TBD — deploying to Render, link goes here once live._

## Why this design

- **The concurrency is real, not simulated.** `feed_sorter_cli` splits the
  feed into chunks, sorts each chunk on its own `std::thread`, then merges
  the sorted chunks with `std::inplace_merge`. Nothing in the browser or
  Node layer sorts anything; they only display what the C++ binary computed.
- **Express is a thin, honest boundary.** `server.js` validates and clamps
  input, spawns the binary via `execFile` (never `exec`/shell interpolation,
  so there's no command-injection surface from user input), parses its JSON
  stdout, and returns it. That's the whole API.
- **The frontend visualizes real chunk provenance.** Every post in the API
  response carries a `chunk` field — which thread's chunk it was sorted in
  before the merge phase. The "Run parallel sort demo" button reconstructs
  the pre-merge state from that field (valid because `std::inplace_merge` is
  stable — see the comment in `public/script.js`) and animates chunk-sort →
  merge using real data, not a scripted fake.

## Project layout

```
backend/
  src/
    Post.h / Post.cpp            — post data model + composite ranking score
    FeedManager.h / .cpp         — thread-safe ingestion + parallel merge-sort
    cli.cpp                      — CLI entry point: generates posts, sorts, emits JSON
  server.js                      — Express API, spawns the compiled binary
  public/                        — static frontend (vanilla JS, no framework)
  Dockerfile                     — installs g++, builds the binary, runs the server
  render.yaml                    — Render Blueprint config
  DEPLOYMENT.md                  — deployment steps
  package.json
```

## Running it locally

Requires Node 18+ and a C++17 compiler (g++ recommended; on Windows, use
WSL — MinGW's default thread model does not implement `<thread>`/`<mutex>`).

```bash
cd backend
npm install
npm run build:cpp     # compiles src/Post.cpp, FeedManager.cpp, cli.cpp -> src/feed_sorter_cli
npm start              # serves API + frontend on :3001
```

Open `http://localhost:3001`.

To verify the API directly:
```bash
curl -X POST http://localhost:3001/api/sort \
  -H "Content-Type: application/json" \
  -d '{"count":10,"threads":3,"mode":"likes","seed":5}'
```

## API

`POST /api/sort`
```json
{ "count": 14, "threads": 4, "mode": "score", "seed": 42 }
```
`mode` is one of `time`, `likes`, `priority`, `score`. Response:
```json
{
  "mode": "score",
  "threadsRequested": 4,
  "chunkCount": 4,
  "hardwareConcurrency": 8,
  "timing": { "serialMs": 0.24, "parallelMs": 0.09 },
  "posts": [ { "id": 0, "userId": "...", "likes": 0, "timestamp": 0, "priority": 0, "score": 0, "chunk": 0 } ]
}
```

## Deployment

See `backend/DEPLOYMENT.md` for full step-by-step Render deployment
instructions (free tier, no credit card, Docker-based build).

## What to say about this in an interview

- Why `execFile` and not `exec`: avoids shell interpolation entirely, so
  arguments are passed as an argv array — no injection risk even though
  input is already validated/clamped before it reaches the binary.
- Why the C++ binary is stateless per-invocation (generates its own data
  rather than reading a shared feed): keeps the process boundary simple and
  avoids needing IPC/shared memory between Node and C++ for this scope. A
  production version would more likely expose the sort as a persistent
  service (gRPC or a long-lived process with a pipe protocol) rather than
  spawning a new process per request — worth mentioning as a known tradeoff
  if asked "how would you scale this."
- A real concurrency bug was found and fixed during development: the
  original parallel-sort implementation shared a `std::function` comparator
  across worker threads by reference. This passed testing on a single-core
  sandbox environment but caused heap corruption ("free(): invalid size")
  reliably on real multi-core hardware. Root cause was narrowed down by
  comparing debug (`-O0`) vs. optimized (`-O2`) builds and confirming the
  crash was specific to genuine multi-core scheduling. Fixed by replacing
  the shared type-erased callable with a plain `SortMode` enum value copied
  into each thread — eliminating any ambiguity about concurrent invocation
  of a shared object, verified stable across 500+ stress-test runs on real
  16-core hardware afterward.