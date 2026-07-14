#pragma once
#include <vector>
#include <mutex>
#include <string>
#include "Post.h"

struct ChunkedResult {
    std::vector<Post> posts;   // final sorted posts
    std::vector<int> chunkOf;  // chunkOf[i] = which chunk post at final index i started in (pre-merge)
    unsigned chunkCount = 0;
    double copyInMs = 0, buildMs = 0, sortMs = 0, mergeMs = 0, copyOutMs = 0;
};

class FeedManager {
private:
    std::vector<Post> allPosts;
    mutable std::mutex feedMutex;

    static std::vector<size_t> computeChunkBounds(size_t n, unsigned numThreads);

public:
    void addPost(const Post& post);
    size_t size() const;

    // Mutex-protected copy of the current posts, unsorted. Exists so
    // callers can time JUST a sort operation on an already-in-memory
    // array, excluding both data generation and this copy itself, for a
    // fair apples-to-apples comparison against the parallel sort's
    // internal sortMs (which also excludes those same setup costs).
    std::vector<Post> getRawCopy() const;

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