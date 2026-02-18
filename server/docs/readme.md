# Server Architecture

The server architecture consists of two main entities:

- **Session Orchestrator Process**
- **Child Game Process**

## High-Level Architecture Diagram 

<img src="diagrams/gen-serv-arch.png" width="1100" height="650">

---

# Session Orchestrator Process

The Session Orchestrator is a single parent process responsible for coordinating game sessions.

## Responsibilities

- Establish and maintain **TCP communication** with clients
- Form games and fill them with players
- Spawn a **Child Game Process** for each active game
- Maintain a queue of players waiting to join a game
- Maintain a queue of available ports for child processes
- Enforce a cap on the number of concurrent game processes (resource limitation)

Once a game is created:

1. The orchestrator assigns an available port to the child process.
2. The assigned port is sent to all participating clients.
3. The TCP connection between orchestrator and client is closed.
4. All further communication happens directly between clients and the Child Game Process using **UDP**.

---

## I/O Model

The orchestrator uses **epoll** to efficiently monitor and retrieve client input over TCP connections.

---

## Client Connection Flow

1. A client connects to the server via TCP.
2. The server:
   - Opens a new socket for the connection
   - Adds the socket to `epoll` monitoring
   - Creates a `Client` structure
   - Stores it in a hashmap:  
     `fd -> Client Structure`
3. The client sends an initialization message indicating readiness.
4. The client is added to the matchmaking queue (FIFO).
5. Once enough players are available:
   - The game is created 
   - A Child Game Process is spawned
   - An available port is assigned
   - The port is sent to all players
6. The orchestrator closes the TCP connection.
7. Clients communicate directly with the Child Game Process over UDP.

---

## Managing Available Ports

Port management is handled through a ring-implemented queue.

- The orchestrator maintains a queue of **N available ports**
- Each new game consumes one port from the queue
- The maximum number of concurrent games is limited to N
- If no ports are available:
  - The orchestrator enters a waiting state
  - It resumes once a child process terminates and returns its port
- When a child process exits:
  - The freed port is pushed back into the queue
  - It becomes available for a new game session

---

## Summary

The orchestrator acts as:

- A TCP entry point
- A matchmaking coordinator
- A controlled game process spawner
- A resource manager (ports + process limit)

All real-time gameplay is handled exclusively by isolated Child Game Processes over UDP.