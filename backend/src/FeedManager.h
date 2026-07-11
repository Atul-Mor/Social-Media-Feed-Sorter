#pragma once
#include <vector>
#include <mutex>
#include <string>
#include "Post.h"

struct ChunkedResult {
    std::vector<Post> posts;
    std::vector<int> chunkOf;
    unsigned chunkCount;
};

class FeedManager {
private:
    std::vector<Post> allPosts;
    mutable std::mutex feedMutex;

    static std::vector<size_t> computeChunkBounds(size_t n, unsigned numThreads);
public:
    void addPost(const Post& post);
    size_t size() const;

    // Single-threaded baselines
    std::vector<Post> getFeedSortedByTime() const;
    std::vector<Post> getFeedSortedByLikes() const;
    std::vector<Post> getFeedSortedByPriority() const;
    std::vector<Post> getFeedSortedByScore(long nowTs) const;

    // Real multi-threaded parallel sort, works for any of the 4 modes.
    // mode: "time" | "likes" | "priority" | "score"
    // Returns the sorted feed plus a per-post record of which chunk it
    // started in, so the caller can visualize the chunk/merge phases.
    ChunkedResult getFeedSortedParallel(const std::string& mode, long nowTs, unsigned numThreads) const;

    void simulateConcurrentIngestion(unsigned numThreads, size_t postsPerThread);
};