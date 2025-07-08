
# Social Media Feed Sorter

A C++ project that simulates how a social media platform sorts user posts based on:
- Recency
- Popularity (likes)
- User-defined priority (e.g., close friends > followers > others)

## Features
- Efficient sorting using STL and custom comparators
- Demonstrates use of DSA concepts: arrays, sorting, maps, heaps (optional)

## DSA Concepts Used
- Vectors
- Custom Sorting with Comparators
- Map for user priority
- (Optional) Priority Queue

## How to Run
```
g++ src/*.cpp -o feed_sorter
./feed_sorter
```

## Sample Output
```
Feed Sorted by Time:
3 by user789 with 50 likes
2 by user456 with 200 likes
...

Feed Sorted by User Priority:
2 by user456 [priority 2]
1 by user123 [priority 1]
...
```
