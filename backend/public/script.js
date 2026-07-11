const CHUNK_COLORS = [
  { bg: '#e1f5ee', border: '#0f6e56', text: '#085041', label: 'Chunk A' },
  { bg: '#eeedfe', border: '#534ab7', text: '#3c3489', label: 'Chunk B' },
  { bg: '#faece7', border: '#993c1d', text: '#712b13', label: 'Chunk C' },
];

async function fetchSort(mode, threads) {
  const res = await fetch('/api/sort', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ count: 14, threads, mode, seed: Math.floor(Math.random() * 100000) }),
  });
  if (!res.ok) throw new Error('sort request failed');
  return res.json();
}

function cardHTML(post, showChunk) {
  const c = CHUNK_COLORS[post.chunk % CHUNK_COLORS.length];
  const initials = post.userId.slice(0, 2).toUpperCase();
  const minsAgo = Math.max(1, Math.round((Date.now() / 1000 - post.timestamp) / 60));
  const priorityLabel = ['Low', 'Medium', 'High'][post.priority];
  return `<div id="card-${post.id}" class="card" style="border-left-color:${showChunk ? c.border : 'transparent'}">
    ${showChunk ? `<div class="chunk-badge" style="background:${c.bg};color:${c.text}">${c.label}</div>` : ''}
    <div class="top">
      <div class="avatar">${initials}</div>
      <div class="name">${post.userId}</div>
    </div>
    <div class="stats">
      <span>&hearts; ${post.likes}</span>
      <span>${priorityLabel}</span>
    </div>
    <div class="time">${minsAgo}m ago</div>
  </div>`;
}

function render(posts, showChunk) {
  document.getElementById('grid').innerHTML = posts.map(p => cardHTML(p, showChunk)).join('');
}

function flipTo(newPosts, showChunk) {
  const grid = document.getElementById('grid');
  const first = new Map([...grid.children].map(el => [el.id, el.getBoundingClientRect()]));
  render(newPosts, showChunk);
  [...grid.children].forEach(el => {
    const f = first.get(el.id);
    if (!f) return;
    const l = el.getBoundingClientRect();
    const dx = f.left - l.left, dy = f.top - l.top;
    el.style.transform = `translate(${dx}px, ${dy}px)`;
    el.style.transition = 'none';
    requestAnimationFrame(() => {
      el.style.transition = 'transform 0.4s ease';
      el.style.transform = 'translate(0, 0)';
    });
  });
}

function setStatus(text) {
  document.getElementById('status').textContent = text;
}

async function doSort(mode) {
  setStatus('Calling the C++ engine...');
  try {
    const data = await fetchSort(mode, 4);
    setStatus(`Sorted by ${mode} — engine reported ${data.timing.parallelMs.toFixed(2)}ms across ${data.chunkCount} thread(s).`);
    flipTo(data.posts, false);
  } catch (e) {
    setStatus('Request failed — is the backend running?');
  }
}

// Reconstructs the pre-merge, chunk-sorted state from the final sorted
// result. Valid because std::inplace_merge is stable: within any given
// chunk, relative order is preserved across the merge phase. Grouping the
// final result by chunk (in the order it appears) exactly reproduces what
// each chunk looked like right after its own thread finished sorting it.
function reconstructChunkPhase(posts, chunkCount) {
  const groups = Array.from({ length: chunkCount }, () => []);
  posts.forEach(p => groups[p.chunk].push(p));
  return groups.flat();
}

async function runParallelDemo() {
  setStatus('Calling the C++ engine...');
  let data;
  try {
    data = await fetchSort('likes', 3);
  } catch (e) {
    setStatus('Request failed — is the backend running?');
    return;
  }

  const chunkPhase = reconstructChunkPhase(data.posts, data.chunkCount);
  setStatus(`Chunks sorted in parallel across ${data.chunkCount} threads...`);
  flipTo(chunkPhase, true);
  await sleep(1100);

  setStatus('Merging sorted chunks (tree merge)...');
  await sleep(700);
  flipTo(data.posts, false);
  await sleep(400);

  setStatus(`Done — real engine time: ${data.timing.parallelMs.toFixed(2)}ms (vs ${data.timing.serialMs.toFixed(2)}ms single-threaded on this machine).`);
}

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

doSort('likes');