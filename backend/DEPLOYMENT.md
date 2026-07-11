# Deploying to Render (free tier)

No credit card required. Free web services get 750 instance-hours/month,
which is far more than a portfolio project needs — the only catch is the
service spins down after 15 minutes of no traffic and takes 30-60s to wake
back up on the next request. Fine for a resume link; just know that if a
recruiter clicks it cold, the first load will be slow.

## 1. Push this code to GitHub

Make sure `backend/Dockerfile`, `backend/render.yaml`, and everything under
`backend/` (server.js, src/, public/, package.json) is committed and pushed.

## 2. Create the Render account

Go to https://render.com → sign up with GitHub (no card needed).

## 3. Create the web service

- Dashboard → **New** → **Web Service**
- Connect your GitHub repo (`Social-Media-Feed-Sorter` or wherever you push
  the `backend/` folder)
- Render should auto-detect the `Dockerfile`. If it asks for a runtime,
  choose **Docker** explicitly.
- **Root directory**: set this to `backend` if your Dockerfile lives inside
  a subfolder of the repo (it does, per this project's layout) — otherwise
  Render will look for the Dockerfile at the repo root and fail to find it.
- **Instance type**: Free
- Click **Create Web Service**

Render will build the Docker image (installs g++, runs `npm install`,
compiles `feed_sorter_cli`, starts the server) and give you a URL like
`https://feed-sorter-xxxx.onrender.com`.

## 4. Using the Blueprint instead (optional, faster)

If you'd rather not click through the UI: with `render.yaml` committed,
go to **New** → **Blueprint**, point it at your repo, and Render reads the
config automatically. Same result, fewer clicks.

## 5. Verify it's live

```bash
curl https://YOUR-URL.onrender.com/api/health
```
Should return `{"ok":true}`. Then open the URL in a browser to see the
frontend.

## 6. Update your resume / README

Swap the old static GitHub Pages "Live" link for this new URL. Note in your
project README that the free-tier instance sleeps after inactivity — that's
honest, and it also happens to be a good interview talking point ("how would
you avoid this in production" → keep-alive ping, or a paid always-on plan,
or a serverless cold-start architecture instead).

## Known limitation worth knowing for an interview

Render's free web services get 0.1 shared vCPU. Your parallel sort will
still run correctly, but you won't see meaningful multi-core speedup on
this specific host, the same way you wouldn't in this sandbox — it's a
resource-constrained shared environment, not a bug in your code. If asked,
say so directly: correctness and thread-safety are proven either way;
speedup numbers are a function of the host's real core count.