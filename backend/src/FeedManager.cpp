#include "FeedManager.h"
#include <algorithm>
#include <thread>
#include <random>
#include <ctime>
#include <functional>

void FeedManager::addPost(const Post& post) {
    std::lock_guard<std::mutex> lock(feedMutex);
    allPosts.push_back(post);
}

size_t FeedManager::size() const {
    std::lock_guard<std::mutex> lock(feedMutex);
    return allPosts.size();
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
    std::vector<Post> feed;
    {
        std::lock_guard<std::mutex> lock(feedMutex);
        feed = allPosts;
    }

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

    std::vector<int> originChunk(n);
    for (size_t c = 0; c < chunkCount; ++c)
        for (size_t i = bounds[c]; i < bounds[c + 1]; ++i)
            originChunk[i] = static_cast<int>(c);

    std::vector<std::pair<Post, int>> tagged;
    tagged.reserve(n);
    for (size_t i = 0; i < n; ++i) tagged.emplace_back(feed[i], originChunk[i]);

    std::vector<std::thread> workers;
    workers.reserve(chunkCount);
    for (size_t c = 0; c < chunkCount; ++c) {
        size_t lo = bounds[c];
        size_t hi = bounds[c + 1];
        workers.emplace_back([&tagged, lo, hi, mode, nowTs]() {
            std::sort(tagged.begin() + lo, tagged.begin() + hi,
                      [mode, nowTs](const std::pair<Post,int>& a, const std::pair<Post,int>& b) {
                          return comparePosts(a.first, b.first, mode, nowTs);
                      });
        });
    }
    for (auto& t : workers) t.join();

    for (size_t step = 1; step < chunkCount; step *= 2) {
        for (size_t i = 0; i + step < chunkCount; i += 2 * step) {
            size_t left = bounds[i];
            size_t mid = bounds[i + step];
            size_t rightIdx = std::min(i + 2 * step, chunkCount);
            size_t right = bounds[rightIdx];
            std::inplace_merge(tagged.begin() + left, tagged.begin() + mid, tagged.begin() + right,
                                [mode, nowTs](const std::pair<Post,int>& a, const std::pair<Post,int>& b) {
                                    return comparePosts(a.first, b.first, mode, nowTs);
                                });
        }
    }

    result.posts.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        result.posts.push_back(tagged[i].first);
        result.chunkOf[i] = tagged[i].second;
    }
    result.chunkCount = chunkCount;
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
