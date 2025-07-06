
const posts = [
  { postId: 1, userId: "user123", likes: 120, timestamp: 1720200000, priority: 1 },
  { postId: 2, userId: "user456", likes: 200, timestamp: 1720203000, priority: 2 },
  { postId: 3, userId: "user789", likes: 50,  timestamp: 1720208000, priority: 0 },
  { postId: 4, userId: "alice",   likes: 75,  timestamp: 1720210000, priority: 1 },
  { postId: 5, userId: "bob",     likes: 180, timestamp: 1720213000, priority: 2 },
  { postId: 6, userId: "carol",   likes: 90,  timestamp: 1720216000, priority: 0 },
  { postId: 7, userId: "dave",    likes: 250, timestamp: 1720219000, priority: 1 },
  { postId: 8, userId: "eve",     likes: 60,  timestamp: 1720222000, priority: 2 },
  { postId: 9, userId: "frank",   likes: 110, timestamp: 1720225000, priority: 1 },
  { postId: 10, userId: "grace",  likes: 30,  timestamp: 1720228000, priority: 0 },
  { postId: 11, userId: "heidi",  likes: 140, timestamp: 1720231000, priority: 2 },
  { postId: 12, userId: "ivan",   likes: 95,  timestamp: 1720234000, priority: 1 },
  { postId: 13, userId: "judy",   likes: 70,  timestamp: 1720237000, priority: 0 },
  { postId: 14, userId: "mallory",likes: 160, timestamp: 1720240000, priority: 1 },
  { postId: 15, userId: "trent",  likes: 45,  timestamp: 1720243000, priority: 2 }
];

function sortFeed(criteria) {
  let sorted = [...posts];

  if (criteria === 'time') {
    sorted.sort((a, b) => b.timestamp - a.timestamp);
  } else if (criteria === 'likes') {
    sorted.sort((a, b) => b.likes - a.likes);
  } else if (criteria === 'priority') {
    sorted.sort((a, b) => b.priority - a.priority || b.timestamp - a.timestamp);
  }

  displayFeed(sorted);
}

function displayFeed(posts) {
  const feed = document.getElementById("feed");
  feed.innerHTML = "";

  posts.forEach((post, index) => {
    const card = document.createElement("div");
    card.className = "card";
    card.style.background = gradients[index % gradients.length];

    card.innerHTML = `
      <h3>Post #${post.postId}</h3>
      <p><strong>User:</strong> ${post.userId}</p>
      <p><strong>Likes:</strong> ${post.likes}</p>
      <p><strong>Priority:</strong> ${post.priority}</p>
      <p><strong>Timestamp:</strong> ${new Date(post.timestamp * 1000).toLocaleString()}</p>
    `;

    feed.appendChild(card);
  });
}

const gradients = [
  "linear-gradient(135deg, #f6d365 0%, #fda085 100%)",
  "linear-gradient(135deg, #a1c4fd 0%, #c2e9fb 100%)",
  "linear-gradient(135deg, #ffecd2 0%, #fcb69f 100%)",
  "linear-gradient(135deg, #d4fc79 0%, #96e6a1 100%)",
  "linear-gradient(135deg, #84fab0 0%, #8fd3f4 100%)",
  "linear-gradient(135deg, #fccb90 0%, #d57eeb 100%)",
  "linear-gradient(135deg, #f093fb 0%, #f5576c 100%)",
  "linear-gradient(135deg, #c3cfe2 0%, #c3cfe2 100%)"
];

// Initial render
sortFeed('time');
