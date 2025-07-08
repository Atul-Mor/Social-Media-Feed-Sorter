
#include "FeedManager.h"
#include <algorithm>

void FeedManager::addPost(Post post) {
    allPosts.push_back(post);
}

std::vector<Post> FeedManager::getFeedSortedByTime() {
    auto feed = allPosts;
    std::sort(feed.begin(), feed.end(), [](Post& a, Post& b) {
        return a.timestamp > b.timestamp;
    });
    return feed;
}

std::vector<Post> FeedManager::getFeedSortedByLikes() {
    auto feed = allPosts;
    std::sort(feed.begin(), feed.end(), [](Post& a, Post& b) {
        return a.likes > b.likes;
    });
    return feed;
}

std::vector<Post> FeedManager::getFeedSortedByPriority() {
    auto feed = allPosts;
    std::sort(feed.begin(), feed.end(), [](Post& a, Post& b) {
        if (a.userPriority != b.userPriority)
            return a.userPriority > b.userPriority;
        return a.timestamp > b.timestamp;
    });
    return feed;
}
