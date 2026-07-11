# SocialMediaFeedSorter

This repository contains a minimal full-stack prototype for sorting and ranking a social-media feed using a lightweight Express backend and a C++ processing component.

## Architecture

- Backend API: Express server in `backend/server.js`
- Ranking logic: C++ implementation under `backend/src/`
- Frontend: Static files served from `backend/public/`
- Deployment: Render-ready configuration in `backend/render.yaml`

## Structure

```text
SocialMediaFeedSorter/
├── README.md
└── backend/
    ├── Dockerfile
    ├── .dockerignore
    ├── render.yaml
    ├── DEPLOYMENT.md
    ├── package.json
    ├── package-lock.json
    ├── server.js
    ├── src/
    │   ├── Post.h
    │   ├── Post.cpp
    │   ├── FeedManager.h
    │   ├── FeedManager.cpp
    │   └── cli.cpp
    └── public/
        ├── index.html
        ├── style.css
        └── script.js
```

## Getting started

1. Install dependencies:
   ```bash
   cd backend
   npm install
   ```
2. Build the C++ binary:
   ```bash
   g++ -std=c++17 src/Post.cpp src/FeedManager.cpp src/cli.cpp -O2 -o bin/feed-sorter
   ```
3. Start the server:
   ```bash
   npm start
   ```

The API accepts a JSON payload of posts and returns ranked results.
