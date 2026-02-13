# Crumbling Coffers  

---

## Table of Contents
* [Description](#description)
* [General Info](#general-information)
* [Technologies Used](#technologies-used)
* [Features](#features)
* [Screenshots](#screenshots)
* [Sprint 1](#sprint-1)

---

## Description

**Team:** Aurora Blakely, Andrea Gomez, Daniel Pelley, Nicholas Krustchinsky, Vadym Tovkach  

**Concept:** A competitive multiplayer Metroidvania-style platformer centered around nonlinear exploration, strategic item collection, and time-based scoring. Players navigate interconnected levels filled with movement challenges, hidden paths, and unlockable abilities that expand traversal options as the match progresses. Each session emphasizes speed, routing efficiency, and smart decision-making, rewarding players not just for collecting items, but for optimizing their path and adapting to opponents in real time. The result is a fast-paced experience that blends precision platforming with competitive depth and replayability.  

**Audience:** Fans of metroidvania platformers and pixel art who enjoy  

**Purpose:** To create a competitive and visually engaging game for friends  

---

## General Information

**Crumbling Coffers** is a multiplayer platformer inspired by Metroidvania design principles.

The game features a large vertically explorable map filled with caves and interconnected areas. Players can traverse upward and downward freely, discovering items scattered across the environment.

The core objective is competitive collection:
- Up to **4 active players** compete within the same map.
- Items have different **rarity tiers** and **score values**.
- Matches are **time-based**.
- After time expires, a **scoreboard** displays all collected items and calculates each player's total score.

The game also includes temporary **boost items (spells)** that enhance abilities such as stamina, speed, and jump height, adding strategic depth to exploration and competition.

--- 

## Features

- Large vertically and horizontally explorable map  
- Multiplayer support (planned: 4 players per match)  
- Item system with rarity tiers and score values  
- Timed competitive matches  
- End-of-match scoreboard with score calculation  
- Temporary boost items affecting player abilities 

**User Stories:**

- Aurora: Map Design & Interactivity (creating platforms & interactable surfaces)  
- Aurora: Match Timer (start match from menu, end matches, navigate to score screen)  

- Andrea: Learning Godot Engine (navigating the engine & using assets)  
- Andrea: Asset creations (custom map tiles, character, and items)  

- Daniel: Learning GDScript (learn technologies needed such as GDScript, C, SQLite)  
- Daniel: Adding and Implementing Ability Items (item design & interactivity, player picking up & using items)  

- Nicholas: Player character movement (player character design & mechanics, player movement, navigating the map)  
- Nicholas: Off-screen visual hints (Off screen direction, reveal other players and rare items)  

- Vadym: Server foundation creation (foundational asynchronous UDP networking layer)  
- Vadym: Create the foundation for the client networking module (UDP send/receive + protocol parsing, main menu before the match starts waiting on other players)  

- Score screen after match ends (compare scores & announce winner)  

---

## Technologies Used

- **Godot Engine** – Rendering, physics, scene management, and cross-platform support.
- **GDScript (Client)** – Gameplay logic, input handling, UI, client-side prediction and interpolation.
- **C (Dedicated Server)** – Authoritative server handling networking, simulation ticks, session management, and state synchronization.
- **SQLite** – Embedded database for storing player accounts, statistics, progression, and leaderboard data.
 
---

## Screenshots (Initial Vision)

<img src="docs/screenshots/ss1.png" width="600">
<img src="docs/screenshots/ss2.png" width="600">
<img src="docs/screenshots/ss3.png" width="600">

---

## Sprint 1

- Learning Godot Engine  
- Leaning GDScript  
- Map Design & Interactivity  
- Player Character Movement  
