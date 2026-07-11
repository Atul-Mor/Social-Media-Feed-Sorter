#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <chrono>
#include <ctime>
#include <thread>
#include "Post.h"
#include "FeedManager.h"

// Minimal arg parser: expects --key=value pairs.
static std::string getArg(int argc, char** argv, const std::string& key, const std::string& def) {
    std::string prefix = "--" + key + "=";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind(prefix, 0) == 0) return a.substr(prefix.size());
    }
    return def;
}

static const char* NAMES[] = {
    "nova_lee","kai_jordan","mira_saito","theo_park","zuri_belle","finn_okoro",
    "asha_ray","luca_moss","ivy_chen","remy_das","wren_cole","aya_singh",
    "dex_ford","nia_wolfe","kian_ross","tara_vex","milo_cruz","sage_ito"
};
static const int NAMES_COUNT = sizeof(NAMES) / sizeof(NAMES[0]);

static std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

int main(int argc, char** argv) {
    int count = std::stoi(getArg(argc, argv, "count", "16"));
    unsigned threads = static_cast<unsigned>(std::stoi(getArg(argc, argv, "threads", "4")));
    std::string mode = getArg(argc, argv, "mode", "likes");
    unsigned seed = static_cast<unsigned>(std::stoul(getArg(argc, argv, "seed", "42")));

    long nowTs = std::time(nullptr);

    FeedManager manager;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> likesDist(0, 5000);
    std::uniform_int_distribution<int> priorityDist(0, 2);
    std::uniform_int_distribution<int> ageDist(60, 6 * 3600);
    std::uniform_int_distribution<int> nameDist(0, NAMES_COUNT - 1);

    for (int i = 0; i < count; ++i) {
        std::string user = NAMES[nameDist(rng)];
        manager.addPost(Post(i, user, likesDist(rng), nowTs - ageDist(rng), priorityDist(rng)));
    }

    // Real timing: single-threaded baseline vs. actual multi-threaded run.
    auto t0 = std::chrono::high_resolution_clock::now();
    std::vector<Post> serial;
    if (mode == "time") serial = manager.getFeedSortedByTime();
    else if (mode == "priority") serial = manager.getFeedSortedByPriority();
    else if (mode == "score") serial = manager.getFeedSortedByScore(nowTs);
    else serial = manager.getFeedSortedByLikes();
    auto t1 = std::chrono::high_resolution_clock::now();
    double serialMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    auto t2 = std::chrono::high_resolution_clock::now();
    ChunkedResult result = manager.getFeedSortedParallel(mode, nowTs, threads);
    auto t3 = std::chrono::high_resolution_clock::now();
    double parallelMs = std::chrono::duration<double, std::milli>(t3 - t2).count();

    // Emit JSON.
    std::ostringstream out;
    out << "{";
    out << "\"mode\":\"" << jsonEscape(mode) << "\",";
    out << "\"threadsRequested\":" << threads << ",";
    out << "\"chunkCount\":" << result.chunkCount << ",";
    out << "\"hardwareConcurrency\":" << std::thread::hardware_concurrency() << ",";
    out << "\"timing\":{\"serialMs\":" << serialMs << ",\"parallelMs\":" << parallelMs << "},";
    out << "\"posts\":[";
    for (size_t i = 0; i < result.posts.size(); ++i) {
        const Post& p = result.posts[i];
        if (i) out << ",";
        out << "{"
            << "\"id\":" << p.postId << ","
            << "\"userId\":\"" << jsonEscape(p.userId) << "\","
            << "\"likes\":" << p.likes << ","
            << "\"timestamp\":" << p.timestamp << ","
            << "\"priority\":" << p.userPriority << ","
            << "\"score\":" << p.rankScore(nowTs) << ","
            << "\"chunk\":" << result.chunkOf[i]
            << "}";
    }
    out << "]}";

    std::cout << out.str() << std::endl;
    return 0;
}