
#pragma once
#include <vector>
#include "Post.h"

class FeedManager {
private:
    std::vector<Post> allPosts;

public:
    void addPost(Post post);
    std::vector<Post> getFeedSortedByTime();
    std::vector<Post> getFeedSortedByLikes();
    std::vector<Post> getFeedSortedByPriority();
};
