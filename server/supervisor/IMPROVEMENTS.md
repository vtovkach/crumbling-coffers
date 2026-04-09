# Improvements

## Event Handling
- Introduce a dispatch routine for processing events  
    - Replace multiple `if/else` chains with a centralized dispatcher

## Project Structure
- Restructure file layout into logical directories  
  - Improve organization without over-structuring or fragmentation

## EventFD Abstraction
- Encapsulate `send_eventfd` and `recv_eventfd`  
- Move associated FDs and queues into a dedicated structure  
- Provide a clear, minimal interface for interaction

## Epoll Utilities
- Create a utility module for epoll operations:
  - add
  - modify
  - remove  
- Reduce boilerplate and improve readability around `epoll_ctl` usage

## Use internal message queues for incoming and outgoing packets
- Instead of using fixed TCP_SEG_SIZE bytes buffer 
  - Create a queue for incoming data
  - Create a queue for outgoing data

## Extract event handlers from matchmaker and orchestrator to separate file 

## Create a separate standalone module for event tracking to abstract epoll internals and reduce repetitive code

## Design logic to ensure the client is notified of errors and always receives a response