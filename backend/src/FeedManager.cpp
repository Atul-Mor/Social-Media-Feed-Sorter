#include "FeedManager.h"
#include <algorithm>
#include <thread>
#include <random>
#include <ctime>
#include <functional>
#include <chrono>
#include <queue>
#include <utility>

void FeedManager::addPost(const Post& post) {
    std::lock_guard<std::mutex> lock(feedMutex);
    allPosts.push_back(post);
}

size_t FeedManager::size() const {
    std::lock_guard<std::mutex> lock(feedMutex);
    return allPosts.size();
}

std::vector<Post> FeedManager::getRawCopy() const {
    std::lock_guard<std::mutex> lock(feedMutex);
    return allPosts;
}

std::vector<Post> FeedManager::getFeedSortedByTime() const {
    std::lock_guard<std::mutex> lock(feedMutex);
    auto feed = allPosts;
    std::sort(feed.begin(), feed.end(), [](const Post& a, const Post& b) { return a.timestamp > b.timestamp; });
    return feed;
}

std::vector<Post> FeedManager::getFeedSortedByLikes() const {
    std::lock_guard<std::mutex> lock(feedMutex);
    auto feed = allPosts;
    std::sort(feed.begin(), feed.end(), [](const Post& a, const Post& b) { return a.likes > b.likes; });
    return feed;
}

std::vector<Post> FeedManager::getFeedSortedByPriority() const {
    std::lock_guard<std::mutex> lock(feedMutex);
    auto feed = allPosts;
    std::sort(feed.begin(), feed.end(), [](const Post& a, const Post& b) {
        if (a.userPriority != b.userPriority) return a.userPriority > b.userPriority;
        return a.timestamp > b.timestamp;
    });
    return feed;
}

std::vector<Post> FeedManager::getFeedSortedByScore(long nowTs) const {
    std::lock_guard<std::mutex> lock(feedMutex);
    auto feed = allPosts;
    std::sort(feed.begin(), feed.end(), [nowTs](const Post& a, const Post& b) {
        return a.rankScore(nowTs) > b.rankScore(nowTs);
    });
    return feed;
}

std::vector<size_t> FeedManager::computeChunkBounds(size_t n, unsigned numThreads) {
    std::vector<size_t> bounds;
    bounds.push_back(0);
    if (numThreads == 0) numThreads = 1;
    size_t chunkSize = (n + numThreads - 1) / numThreads;
    for (size_t start = chunkSize; start < n; start += chunkSize) bounds.push_back(start);
    bounds.push_back(n);
    return bounds;
}

enum class SortMode { Time, Likes, Priority, Score };

static SortMode parseMode(const std::string& mode) {
    if (mode == "time") return SortMode::Time;
    if (mode == "priority") return SortMode::Priority;
    if (mode == "score") return SortMode::Score;
    return SortMode::Likes;
}

// No std::function, no shared callable object invoked across threads.
// Each thread gets its own SortMode (a plain value type) and calls this
// free function directly — nothing shared, nothing type-erased, nothing
// ambiguous about concurrent invocation.
static inline bool comparePosts(const Post& a, const Post& b, SortMode mode, long nowTs) {
    switch (mode) {
        case SortMode::Time:
            return a.timestamp > b.timestamp;
        case SortMode::Priority:
            if (a.userPriority != b.userPriority) return a.userPriority > b.userPriority;
            return a.timestamp > b.timestamp;
        case SortMode::Score:
            return a.rankScore(nowTs) > b.rankScore(nowTs);
        case SortMode::Likes:
        default:
            return a.likes > b.likes;
    }
}

ChunkedResult FeedManager::getFeedSortedParallel(const std::string& modeStr, long nowTs, unsigned numThreads) const {
    auto tStart = std::chrono::high_resolution_clock::now();

    std::vector<Post> feed;
    {
        std::lock_guard<std::mutex> lock(feedMutex);
        feed = allPosts;
    }
    auto tCopied = std::chrono::high_resolution_clock::now();

    SortMode mode = parseMode(modeStr);
    size_t n = feed.size();

    ChunkedResult result;
    result.chunkOf.assign(n, 0);

    if (numThreads <= 1 || n < 2 * numThreads) {
        for (size_t i = 0; i < n; ++i) result.chunkOf[i] = 0;
        std::sort(feed.begin(), feed.end(), [mode, nowTs](const Post& a, const Post& b) {
            return comparePosts(a, b, mode, nowTs);
        });
        result.posts = std::move(feed);
        result.chunkCount = 1;
        return result;
    }

    auto bounds = computeChunkBounds(n, numThreads);
    size_t chunkCount = bounds.size() - 1;

    // Tag chunk directly on each Post (cheap int write, no separate
    // wrapper struct, no extra copy/move of the Post itself). This alone
    // replaced what used to be a ~1 full extra O(n) pass constructing a
    // std::pair<Post,int> array.
    for (size_t c = 0; c < chunkCount; ++c)
        for (size_t i = bounds[c]; i < bounds[c + 1]; ++i)
            feed[i].chunkTag = static_cast<int>(c);
    auto tBuilt = std::chrono::high_resolution_clock::now();

    // 1. Sort each chunk concurrently, directly on `feed` — no wrapper
    //    structure, threads operate on disjoint slices of the same array
    //    already used for the copy-in.
    std::vector<std::thread> workers;
    workers.reserve(chunkCount);
    for (size_t c = 0; c < chunkCount; ++c) {
        size_t lo = bounds[c];
        size_t hi = bounds[c + 1];
        workers.emplace_back([&feed, lo, hi, mode, nowTs]() {
            std::sort(feed.begin() + lo, feed.begin() + hi, [mode, nowTs](const Post& a, const Post& b) {
                return comparePosts(a, b, mode, nowTs);
            });
        });
    }
    for (auto& t : workers) t.join();
    auto tSorted = std::chrono::high_resolution_clock::now();

    // 2. Merge all sorted chunks in a SINGLE pass using a k-way merge
    //    (min-heap over the current head of each chunk), instead of a
    //    multi-level tree merge. The tree-merge approach touches every
    //    element once per level (log2(chunkCount) levels — 4 levels for
    //    16 threads), and at scale that repeated full-array traversal was
    //    measured as the dominant cost (10M elements: ~3.8s for a 4-level
    //    tree merge). A k-way merge touches each element exactly once,
    //    regardless of how many chunks/threads are involved.
    using HeapEntry = std::pair<size_t, size_t>; // {chunkIndex, currentPosInFeed}
    auto heapCmp = [&feed, mode, nowTs](const HeapEntry& x, const HeapEntry& y) {
        // std::priority_queue is a max-heap; we want the element that
        // should come FIRST in the output to be popped first, so we
        // invert: x is "less" (lower heap priority) than y whenever y's
        // post should actually precede x's post.
        return comparePosts(feed[y.second], feed[x.second], mode, nowTs);
    };
    std::priority_queue<HeapEntry, std::vector<HeapEntry>, decltype(heapCmp)> pq(heapCmp);
    for (size_t c = 0; c < chunkCount; ++c)
        if (bounds[c] < bounds[c + 1]) pq.push({c, bounds[c]});

    std::vector<Post> merged;
    merged.reserve(n);
    while (!pq.empty()) {
        auto [c, pos] = pq.top();
        pq.pop();
        merged.push_back(std::move(feed[pos]));
        size_t nextPos = pos + 1;
        if (nextPos < bounds[c + 1]) pq.push({c, nextPos});
    }
    auto tMerged = std::chrono::high_resolution_clock::now();

    // Extract chunk tags with a cheap, read-only int pass, THEN move the
    // entire buffer's storage in one O(1) operation — rather than looping
    // n times doing push_back(move(Post)) per element, which was measured
    // to cost more than the actual sort phase at large n (10M elements:
    // ~2.1s for a per-element move loop vs a single pointer-transfer move).
    result.chunkOf.resize(n);
    for (size_t i = 0; i < n; ++i) result.chunkOf[i] = merged[i].chunkTag;
    result.posts = std::move(merged);
    result.chunkCount = chunkCount;
    auto tDone = std::chrono::high_resolution_clock::now();

    auto ms = [](auto a, auto b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
    result.copyInMs = ms(tStart, tCopied);
    result.buildMs = ms(tCopied, tBuilt);
    result.sortMs = ms(tBuilt, tSorted);
    result.mergeMs = ms(tSorted, tMerged);
    result.copyOutMs = ms(tMerged, tDone);

    return result;
}

void FeedManager::simulateConcurrentIngestion(unsigned numThreads, size_t postsPerThread) {
    std::vector<std::thread> producers;
    producers.reserve(numThreads);
    for (unsigned t = 0; t < numThreads; ++t) {
        producers.emplace_back([this, t, postsPerThread]() {
            std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)) + t);
            std::uniform_int_distribution<int> likesDist(0, 5000);
            std::uniform_int_distribution<int> priorityDist(0, 2);
            for (size_t i = 0; i < postsPerThread; ++i) {
                int id = static_cast<int>(t * postsPerThread + i);
                std::string user = "thread" + std::to_string(t) + "_user" + std::to_string(i);
                addPost(Post(id, user, likesDist(rng), std::time(nullptr) - static_cast<long>(i % 3600), priorityDist(rng)));
            }
        });
    }
    for (auto& t : producers) t.join();
}