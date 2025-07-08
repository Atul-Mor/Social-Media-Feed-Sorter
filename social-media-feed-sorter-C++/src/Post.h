
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
};
