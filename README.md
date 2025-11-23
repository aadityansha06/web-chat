# Real-Time Web Chat Application

So basically I built this chat app using C for the server and plain HTML/JS for the client. It's pretty simple - just uses HTTP polling to let multiple people chat in real-time.

![Chat Application Screenshot](screenshot.png)

## What it does

The app lets multiple users send messages to each other through a browser. Every client polls the server once per second to check for new messages. It's got CORS enabled so you can test it locally, and there's some basic styling with Tailwind to make it not look terrible.

The server keeps messages in memory and broadcasts them to everyone except the person who sent it. Each client gets a unique ID so the server knows who's who.

## How it's built

Instead of using WebSockets (which would be smarter but harder to implement in C), I went with the polling approach. The client just hammers the server with GET requests every second asking "got anything new for me?" 

The C server listens on port 8080 and handles two types of requests - POST when someone sends a message, and GET when someone wants to receive messages. It stores up to 100 messages in a simple array in memory.

On the client side, each browser tab generates its own UUID and uses that to identify itself. Messages get sent via POST and the polling happens in a setInterval loop.

## What you need

- GCC to compile the C code
- Any modern browser
- That's pretty much it

## Getting it running

First make a directory and put the files in there:

```bash
mkdir web-chat
cd web-chat
```

Then compile the server:

```bash
gcc server.c -o server
```

You might get some warnings about format truncation but don't worry about them.

Start the server:

```bash
./server
```

You should see something like:
```
Socket created successfully with file descriptor: 3
SO_REUSEADDR enabled
Bind successful
Listening on port 8080
```

Now just open chat.html in multiple browser tabs and start typing. Each tab is a different user.

To stop the server just hit Ctrl+C.

## How the messaging works

When User A sends a message, it goes to the server via POST with User A's UUID attached. The server stores it. Then when User B polls the server, the server checks if the message came from User B or not. If not, it sends it to User B. Pretty straightforward.

Here's what a POST request looks like:

```http
POST /send HTTP/1.1
Content-Type: application/x-www-form-urlencoded

message=Hello%20World&client_id=550e8400-e29b-41d4-a716-446655440000
```

And a GET request:

```http
GET /receive?client_id=7c9e6679-7425-40de-944b-e07fc1f90ae7 HTTP/1.1
```

## Code overview

The server code has a main loop that accepts connections and figures out if it's a POST or GET request. POST requests go to sender_msg() which parses the message body, URL-decodes it, and stores it. GET requests go to reciver_msg() (yeah I know it's spelled wrong) which loops through all stored messages and returns the ones that aren't from that client.

The client side is even simpler - there's a sendMessage() function that POSTs, a receiveMessages() function that polls every second, and showMessage() that adds stuff to the UI.

## Config stuff

If you want to change settings, look in server.c for:

```c
#define MAX_MESSAGES 100     // how many messages to keep
#define PORT 8080            // what port to use
```

In chat.html you can change:

```javascript
const SEND_URL = 'http://127.0.0.1:8080/send';
const RECV_URL = 'http://127.0.0.1:8080/receive';

setInterval(receiveMessages, 1000);  // how often to poll
```

## Known issues

There's a bunch of limitations I'm aware of:

- Everything's in memory so restart the server and all messages are gone
- Only keeps 100 messages then starts overwriting
- No timestamps on anything
- When you join you see ALL messages that happened before
- No usernames, just UUIDs nobody can read
- The constant polling is kinda wasteful on bandwidth

## If stuff breaks

**Port already in use error**

Someone's already using port 8080. Find the process and kill it:

```bash
sudo lsof -i :8080
kill -9 <PID>
```

**Messages not showing up**

- Open browser console (F12) and check for errors
- Make sure the server is actually running
- Try opening in different tabs (each needs its own UUID)
- Check the network tab to see if requests are working

**Server crashes**

Probably hit the 100 message limit or there's a buffer overflow somewhere in the URL decoding. Just restart it.

## Things I might add later

Would be cool to add:
- Save messages to a file or database
- Let people pick usernames
- Show when messages were sent
- Add private messaging
- Make different chat rooms
- Switch to WebSockets to stop the polling madness
- Add typing indicators
- Let people send files/images

## Why polling and not WebSockets?

Honestly just because it's easier to code in C. Don't have to deal with the WebSocket handshake and protocol. Yeah it's less efficient and there's like a 1 second delay on messages but for a learning project it's fine.

## Memory stuff

I used static allocation with a fixed array to keep things simple. No malloc/free to worry about. Trade-off is it doesn't scale well but the code is easier to understand.

## Security

This is NOT production ready at all. There's:
- No input validation
- No rate limiting  
- No authentication
- XSS vulnerabilities if you don't escape HTML
- No encryption/HTTPS

Basically don't use this for anything real.

## License

Do whatever you want with it. It's for learning.

## Contributions

If you want to improve it go ahead. Fork it and make it better.

---

I made this to learn about socket programming in C and understand how HTTP actually works under the hood. Also wanted to see the difference between polling and real-time push models.
