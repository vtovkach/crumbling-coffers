# Game Server Process

The Game Server Process contains three concurrent threads.

---

## Net Thread

- Continuously waits for and receives client UDP packets  
- Buffers packets into a shared data structure  
- Never touches or modifies active game state  

---

## Game Simulation Thread

- Reads buffered UDP packets from clients  
- Uses those packets to update the authoritative server game simulation  
- Serializes data and sends game state snapshots to clients  
- The only thread allowed to modify `Game` structures  

---

## Matchmaking Thread

- Maintains players queue who wants to join the game   
- Processes client game join requests  
- Adds players to the queue and forms matches  
- Creates a `Game` structure once players are selected  
- Passes the created game to the Games Storage, where the Game Simulation Thread takes ownership  
- Never touches or modifies the game after it has been handed off to the Game Simulation Thread  

---

## High-Level Architecture Diagram 

<img src="diagrams/game-proc-overview.png" width="600" height="400">