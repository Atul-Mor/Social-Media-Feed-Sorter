#pragma once
#include <string>

class Post {
public:
    int postId;
    std::string userId;
    int likes;
    long timestamp;
    int userPriority;

    Post(int id, std::string user, int likes, long time, int priority);

    // Composite ranking score: blends recency, popularity, and user priority.
    // nowTs is the reference "current time" used to compute recency decay.
    double rankScore(long nowTs) const;
};