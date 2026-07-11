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

static std::function<bool(const Post&, const Post&)> comparatorFor(const std::string& mode, long nowTs) {
    if (mode == "time") return [](const Post& a, const Post& b) { return a.timestamp > b.timestamp; };
    if (mode == "priority") return [](const Post& a, const Post& b) {
        if (a.userPriority != b.userPriority) return a.userPriority > b.userPriority;
        return a.timestamp > b.timestamp;
    };
    if (mode == "score") return [nowTs](const Post& a, const Post& b) { return a.rankScore(nowTs) > b.rankScore(nowTs); };
    return [](const Post& a, const Post& b) { return a.likes > b.likes; }; // default: likes
}

ChunkedResult FeedManager::getFeedSortedParallel(const std::string& mode, long nowTs, unsigned numThreads) const {
    std::vector<Post> feed;
    {
        std::lock_guard<std::mutex> lock(feedMutex);
        feed = allPosts;
    }

    auto cmp = comparatorFor(mode, nowTs);
    size_t n = feed.size();

    ChunkedResult result;
    result.chunkOf.assign(n, 0);

    if (numThreads <= 1 || n < 2 * numThreads) {
        for (size_t i = 0; i < n; ++i) result.chunkOf[i] = 0;
        std::sort(feed.begin(), feed.end(), cmp);
        result.posts = std::move(feed);
        result.chunkCount = 1;
        return result;
    }

    auto bounds = computeChunkBounds(n, numThreads);
    size_t chunkCount = bounds.size() - 1;

    // Tag each post with its origin chunk BEFORE any sorting happens.
    std::vector<int> originChunk(n);
    for (size_t c = 0; c < chunkCount; ++c)
        for (size_t i = bounds[c]; i < bounds[c + 1]; ++i)
            originChunk[i] = static_cast<int>(c);

    // 1. Sort each chunk concurrently, on its own thread. Carry the origin
    //    tags along with the posts (as index pairs) so we can report which
    //    chunk each post came from after everything gets merged.
    std::vector<std::pair<Post, int>> tagged;
    tagged.reserve(n);
    for (size_t i = 0; i < n; ++i) tagged.emplace_back(feed[i], originChunk[i]);

    std::vector<std::thread> workers;
    workers.reserve(chunkCount);
    for (size_t c = 0; c < chunkCount; ++c) {
        workers.emplace_back([&tagged, &bounds, c, &cmp]() {
            std::sort(tagged.begin() + bounds[c], tagged.begin() + bounds[c + 1],
                      [&cmp](const std::pair<Post,int>& a, const std::pair<Post,int>& b) {
                          return cmp(a.first, b.first);
                      });
        });
    }
    for (auto& t : workers) t.join();

    // 2. Merge sorted chunks pairwise (tree merge).
    for (size_t step = 1; step < chunkCount; step *= 2) {
        for (size_t i = 0; i + step < chunkCount; i += 2 * step) {
            size_t left = bounds[i];
            size_t mid = bounds[i + step];
            size_t rightIdx = std::min(i + 2 * step, chunkCount);
            size_t right = bounds[rightIdx];
            std::inplace_merge(tagged.begin() + left, tagged.begin() + mid, tagged.begin() + right,
                                [&cmp](const std::pair<Post,int>& a, const std::pair<Post,int>& b) {
                                    return cmp(a.first, b.first);
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