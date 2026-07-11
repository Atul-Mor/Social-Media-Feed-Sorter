#include "Post.h"
#include <cmath>

Post::Post(int id, std::string user, int likes, long time, int priority)
    : postId(id), userId(std::move(user)), likes(likes), timestamp(time), userPriority(priority) {}

double Post::rankScore(long nowTs) const {
    // Recency: exponential decay with a 6-hour half-life (21600s).
    double ageSeconds = static_cast<double>(nowTs - timestamp);
    double recencyScore = std::exp(-ageSeconds / 21600.0);

    // Popularity: log-dampened so viral posts don't completely dominate.
    double popularityScore = std::log1p(static_cast<double>(likes));

    // Priority: close friends / followed users weighted directly.
    double priorityScore = static_cast<double>(userPriority);

    // Weighted blend — tunable weights.
    return (0.5 * recencyScore) + (0.3 * popularityScore) + (0.2 * priorityScore);
}