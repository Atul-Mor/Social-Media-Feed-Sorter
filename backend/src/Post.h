#pragma once
#include <string>

class Post {
public:
    int postId;
    std::string userId;
    int likes;
    long timestamp;
    int userPriority;

    // Scratch field used only during parallel sort: which chunk this post
    // was assigned to before sorting. Travels with the Post through
    // std::sort/std::merge for free (no separate wrapper struct needed),
    // so the caller can report chunk provenance after merging without any
    // extra bookkeeping structure or extra copies.
    int chunkTag = 0;

    Post(int id, std::string user, int likes, long time, int priority);

    // Composite ranking score: blends recency, popularity, and user priority.
    // nowTs is the reference "current time" used to compute recency decay.
    double rankScore(long nowTs) const;
};