# Real-Time Web Chat Application

A lightweight, polling-based chat application built with C (server) and HTML/JavaScript (client). Multiple users can send and receive messages in real-time through HTTP polling.

![Chat Application Screenshot](screenshot.png)

## Features

- 🔄 **Real-time messaging** using HTTP polling (1-second intervals)
- 👥 **Multi-user support** with unique client identification
- 🌐 **Cross-origin support** (CORS enabled)
- 💬 **Broadcast messaging** - Messages visible to all users except the sender
- 🎨 **Modern UI** with Tailwind CSS
- 🔒 **Clean shutdown** with signal handlers

## Architecture

### Polling Model
Unlike WebSocket-based chat applications, this uses a **polling approach**:
- Client sends GET requests every second to check for new messages
- Server maintains a message queue in memory
- Each message is tagged with sender's UUID
- Messages are broadcast to all clients except the sender

### Components

**Server (C)**
- HTTP server listening on port 8080
- Handles POST requests for sending messages
- Handles GET requests for receiving messages
- Stores up to 100 messages in memory
- URL decoding for special characters

**Client (HTML/JavaScript)**
- Generates unique UUID for each browser tab
- Polls server every second for new messages
- Sends messages via POST requests
- Displays messages in a chat interface

## Prerequisites

- GCC compiler (for C server)
- Modern web browser (Chrome, Firefox, Safari, Edge)
- Basic understanding of HTTP and networking

## Installation

### 1. Clone or Download the Project
```bash
mkdir web-chat
cd web-chat
```

### 2. Save the Files

Save `server.c` and `chat.html` in the same directory.

### 3. Compile the Server
```bash
gcc server.c -o server
```

If you see warnings about format truncation, they're safe to ignore.

## Usage

### Starting the Server
```bash
./server
```

Expected output:
```
Socket created successfully with file descriptor: 3
SO_REUSEADDR enabled
Bind successful
Listening on port 8080
```

### Opening the Chat Client

1. Open `chat.html` in your web browser
2. Open the same file in **multiple browser tabs or windows** to simulate multiple users
3. Start chatting!

### Stopping the Server

Press `Ctrl+C` to gracefully shut down the server.

## How It Works

### Message Flow
```
User A sends "Hello"
    ↓
[POST] → Server stores message with UUID_A
    ↓
User B polls (every second)
    ↓
[GET] → Server checks: UUID_B != UUID_A?
    ↓
Server sends "Hello" to User B
    ↓
User B displays message
```

### Request Examples

**Sending a Message (POST)**
```http
POST /send HTTP/1.1
Content-Type: application/x-www-form-urlencoded

message=Hello%20World&client_id=550e8400-e29b-41d4-a716-446655440000
```

**Receiving Messages (GET)**
```http
GET /receive?client_id=7c9e6679-7425-40de-944b-e07fc1f90ae7 HTTP/1.1
```

**Server Response**
```http
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 12

Hello World
```

## Code Structure

### Server (`server.c`)
```
main()
├── Socket creation & binding
├── Accept connections loop
│   ├── Parse HTTP request (POST/GET)
│   ├── POST → sender_msg()
│   └── GET → reciver_msg()
└── Signal handlers for cleanup

sender_msg()
├── Extract message body
├── Parse message & client_id
├── URL decode message
└── Store in message array

reciver_msg()
├── Loop through all messages
├── Filter out sender's own messages
├── Collect matching messages
└── Send HTTP response
```

### Client (`chat.html`)
```
JavaScript Components
├── UUID generation (crypto.randomUUID)
├── sendMessage() - POST to /send
├── receiveMessages() - GET from /receive every 1s
└── showMessage() - Display in UI
```

## Configuration

### Server Settings

Modify in `server.c`:
```c
#define MAX_MESSAGES 100     // Maximum stored messages
#define PORT 8080            // Server port
```

### Client Settings

Modify in `chat.html`:
```javascript
const SEND_URL = 'http://127.0.0.1:8080/send';
const RECV_URL = 'http://127.0.0.1:8080/receive';

setInterval(receiveMessages, 1000);  // Polling interval (ms)
```

## Limitations

⚠️ **Current Limitations:**
- Messages stored in memory only (lost on server restart)
- No message persistence or history
- Limited to 100 messages in queue
- No message timestamps
- Users see ALL previous messages when joining
- No user authentication or nicknames
- Polling creates constant network traffic

## Troubleshooting

### Port Already in Use
```bash
Error: Bind failed: Address already in use
```

**Solution:** Kill existing process on port 8080
```bash
sudo lsof -i :8080
kill -9 <PID>
```

### Messages Not Appearing

1. Check browser console for errors (F12)
2. Verify server is running and shows debug output
3. Ensure you're using different browser tabs (different UUIDs)
4. Check CORS headers in network tab

### Server Crashes

- Check if message buffer is full (100 messages)
- Look for buffer overflow in URL decoding
- Restart server to clear message queue

## Future Enhancements

- [ ] Message persistence (database or file)
- [ ] User nicknames/usernames
- [ ] Message timestamps
- [ ] Private/direct messaging
- [ ] Chat rooms
- [ ] WebSocket support (eliminate polling)
- [ ] Message delivery confirmation
- [ ] Typing indicators
- [ ] File/image sharing

## Technical Details

### Why Polling Instead of WebSockets?

This implementation uses **HTTP polling** for simplicity:
- ✅ Easier to implement in pure C
- ✅ No need for WebSocket protocol handling
- ✅ Works with standard HTTP
- ❌ Less efficient (constant requests)
- ❌ Higher latency (~1 second delay)

### Memory Management

- **Static allocation**: Fixed 100-message array
- **No dynamic memory**: Avoids malloc/free complexity
- **Trade-off**: Limited scalability but simpler code

### Security Considerations

⚠️ **This is a learning project and NOT production-ready:**
- No input validation or sanitization
- No rate limiting
- No authentication
- Vulnerable to XSS if messages aren't escaped
- No HTTPS/encryption

## License

Free to use for educational purposes.

## Contributing

Feel free to fork and improve! Some ideas:
- Add message timestamps
- Implement user authentication
- Add message persistence
- Improve error handling
- Add unit tests

## Author

Built as a learning project to understand:
- C socket programming
- HTTP protocol fundamentals
- Client-server architecture
- Polling vs. push communication models

---

**Enjoy chatting!** 💬
