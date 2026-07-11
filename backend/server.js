const express = require('express');
const cors = require('cors');
const { execFile } = require('child_process');
const path = require('path');

const app = express();
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

const CLI_PATH = path.join(__dirname, 'src', 'feed_sorter_cli');
const VALID_MODES = new Set(['time', 'likes', 'priority', 'score']);

app.get('/api/health', (req, res) => res.json({ ok: true }));

// POST /api/sort  { count, threads, mode, seed }
// Spawns the real compiled C++ binary, which generates the posts,
// runs an actual multi-threaded parallel merge-sort, and returns the
// sorted feed plus per-post chunk provenance and real timing.
app.post('/api/sort', (req, res) => {
  const count = clamp(parseInt(req.body.count, 10) || 16, 1, 5000);
  const threads = clamp(parseInt(req.body.threads, 10) || 4, 1, 16);
  const mode = VALID_MODES.has(req.body.mode) ? req.body.mode : 'likes';
  const seed = clamp(parseInt(req.body.seed, 10) || 42, 0, 2147483647);

  const args = [
    `--count=${count}`,
    `--threads=${threads}`,
    `--mode=${mode}`,
    `--seed=${seed}`,
  ];

  execFile(CLI_PATH, args, { timeout: 5000, maxBuffer: 10 * 1024 * 1024 }, (err, stdout, stderr) => {
    if (err) {
      console.error('feed_sorter_cli failed:', stderr || err.message);
      return res.status(500).json({ error: 'sort engine failed', detail: stderr || err.message });
    }
    try {
      const result = JSON.parse(stdout);
      res.json(result);
    } catch (parseErr) {
      res.status(500).json({ error: 'could not parse sort engine output', raw: stdout });
    }
  });
});

function clamp(n, lo, hi) {
  return Math.max(lo, Math.min(hi, n));
}

const PORT = process.env.PORT || 3001;
app.listen(PORT, () => console.log(`feed-sorter-backend listening on :${PORT}`));