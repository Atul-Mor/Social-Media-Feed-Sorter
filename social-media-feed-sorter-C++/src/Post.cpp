
#include "Post.h"

Post::Post(int id, std::string user, int likes, long time, int priority)
    : postId(id), userId(user), likes(likes), timestamp(time), userPriority(priority) {}
