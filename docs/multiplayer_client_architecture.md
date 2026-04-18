# Multiplayer Client Architecture

## Overview

When a match is found, a `Game` scene is instantiated. It owns all players and the map, handles all incoming network data, and drives per-frame remote player updates.

There are two types of players:
- **LocalPlayer** — the player on this machine. Moves instantly on input, sends input to the server. No interpolation.
- **RemotePlayer** — all other players. Positions are driven entirely by server snapshots using interpolation/extrapolation.

---

## Scene Tree

```
Game  (root of the multiplayer scene)
├── Map         (tilemap / environment — purely visual and collision geometry)
└── Players     (container node)
    ├── LocalPlayer
    ├── RemotePlayer_1
    └── RemotePlayer_2
```

- **Map** is dumb. It owns no players and has no logic referencing them.
- **Game** owns the Players container. It is the only node that touches NetworkManager.

---

## Game

**Responsibilities:**
- Instantiated when a multiplayer match is found
- Receives init packet from server → spawns one `LocalPlayer` and N `RemotePlayer`s at initial positions
- Knows which player is local and which are remote
- Waits for server start signal before enabling updates
- Every frame (`_process(delta)`): drains NetworkManager, routes packets to the correct RemotePlayer's buffer, then calls `update_position(delta)` on each RemotePlayer

**Pseudocode:**
```gdscript
func _process(delta):
    var packets = NetworkManager.drain_received()
    for packet in packets:
        remote_players[packet.player_id].push_packet(packet)

    for player in remote_players.values():
        player.update_position(delta)
```

**Does not know or care** about interpolation logic — that lives entirely in RemotePlayer.

---

## LocalPlayer

**Responsibilities:**
- Moves instantly on input — no interpolation
- Sends input to the server each frame
- If the server returns a corrected position that differs from the predicted one, snaps to the server position

## RemotePlayer

**Responsibilities:**
- Maintains an internal buffer of position snapshots received from the server
- Velocity is provided directly by the server (`vx`, `vy`) — no need to derive it
- Performs interpolation / extrapolation in `update_position(delta)`
- Sets its own `position` — Godot renders it there automatically, no Map involvement needed

### update_position logic (RemotePlayer only)

1. **Normal case** — two or more snapshots available, interpolate between them:
   ```
   position = lerp(prev_pos, target_pos, t)
   ```
   where `t` advances based on elapsed time relative to the snapshot interval.

2. **Packet late / missed** — extrapolate using last known state:
   ```
   position = last_known_pos + last_known_velocity * time_since_last_packet
   ```
   Safe for 1–3 missed packets. Beyond that, drift becomes significant.

3. **Correction** — when a real packet arrives after extrapolation, lerp back to the true position rather than snapping.

---

## Packet Buffer (inside Player)

- A short queue of `{ x, y, vx, vy, client_timestamp }` snapshots
- `client_timestamp` is set by Game when the packet arrives: `Time.get_ticks_msec()`
- Player always interpolates between two known snapshots
- Acts as a jitter buffer: Player renders slightly behind the server (1–2 snapshot intervals), giving a cushion so a late packet doesn't cause a stutter

---

## Network polling rate

- Game polls NetworkManager in `_process(delta)` — once per rendered frame (~60Hz)
- Server sends position updates at ~60Hz (every ~17ms)
- No timer needed; `_process` is the correct hook

---

## Assumptions (current scope)

- Server guarantees all positions are valid (no wall clipping, no anomalies)
- No client-side collision correction needed
- Map data not consulted during interpolation — can be added later if needed (e.g. wall-aware path correction)

---

## Future extensions

- **Cubic / Hermite interpolation** — smoother curves using velocity at both endpoints
- **Wall-aware interpolation** — Player queries Map for collision geometry to prevent interpolated path clipping through walls
