
#include <iostream>
#include "Post.h"
#include "FeedManager.h"
#include <ctime>

int main() {
    FeedManager manager;

    manager.addPost(Post(1, "user123", 120, time(0) - 500, 1));
    manager.addPost(Post(2, "user456", 200, time(0) - 200, 2));
    manager.addPost(Post(3, "user789", 50,  time(0) - 100, 0));

    std::cout << "\nFeed Sorted by Time:\n";
    for (auto& post : manager.getFeedSortedByTime()) {
        std::cout << post.postId << " by " << post.userId << " with " << post.likes << " likes\n";
    }

    std::cout << "\nFeed Sorted by Likes:\n";
    for (auto& post : manager.getFeedSortedByLikes()) {
        std::cout << post.postId << " by " << post.userId << " with " << post.likes << " likes\n";
    }

    std::cout << "\nFeed Sorted by User Priority:\n";
    for (auto& post : manager.getFeedSortedByPriority()) {
        std::cout << post.postId << " by " << post.userId << " [priority " << post.userPriority << "]\n";
    }

    return 0;
}
