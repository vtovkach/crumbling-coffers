# 	Does not have multiplayer support yet;
#	Need to differentiate between client player	
#	and other connected players in same room 

extends CharacterBody2D
class_name Player

# Baseline values. Player has this by default.
const BASE_MAX_SPEED: float = 10000.0
const BASE_MAX_RUNSPEED: float = 1600.0
const BASE_MAX_FALLINGSPEED: float = 5600.0
const BASE_ACCEL: float = 8000.0
const BASE_DECEL: float = 8000.0
const BASE_BRAKING_DECEL: float = 16000.0
const BASE_JUMP_VELOCITY: float = -3000.0

# Values used in calculation. Can increase or decrease with
# temporary status effects, can reset to BASE value on expiration.
@export var max_speed: float = BASE_MAX_SPEED
@export var max_runspeed: float = BASE_MAX_RUNSPEED
@export var max_fallingspeed: float = BASE_MAX_FALLINGSPEED
@export var accel: float = BASE_ACCEL
@export var decel: float = BASE_DECEL
@export var braking_decel: float = BASE_BRAKING_DECEL
@export var jump_velocity: float = BASE_JUMP_VELOCITY

# Values for states
@export var direction: float = 0
@export var jump_pressed: bool = false

# DO NOT MODIFY outside of set_inverted(bool)
var _invert_multiplier: int = 1

@onready var state_machine = $StateMachine
@onready var idle_state = preload("uid://cq3twrefhj7a0").new()
@onready var run_state  = preload("uid://upf8fro5n3lg").new()
@onready var fall_state = preload("uid://cx4qvh51f854a").new()

@export var inv: Inv

#Daniel - adding a score to the character for when they pick up the items.

signal score_changed(new_score: int)

var score: int = 0

func add_score(amount: int) -> void:
	score += amount
	emit_signal("score_changed", score)
# Daniel - ending here

func _physics_process(delta: float) -> void:

	# Update up/down velocity
	if not is_on_floor():
		apply_gravity(delta)
	
	if (Input.is_action_pressed("jump")):
		jump_pressed = true
	else:
		jump_pressed = false
	
	# Update left/right velocity
	direction = _invert_multiplier * Input.get_axis("left", "right")
	move(direction, delta)
		
	# Update position
	move_and_slide()

func jump() -> void:
	if is_on_floor():
		velocity.y = jump_velocity
		
func apply_gravity(delta: float) -> void:
	velocity.y = move_toward(velocity.y, max_fallingspeed, get_gravity().y * delta)

# Update velocity according to direction of movement
# direction: a float in [-1, 1]
# [-1, 0): 	To left.
# 0: 		Stop 
# (0, 1]:	To right.
func move(direction: float, delta: float) -> void:
	if direction:
		if (velocity.x > 0 and direction < 0) or (velocity.x < 0 and direction > 0):
			brake(direction, delta)
		else:
			velocity.x = move_toward(velocity.x, direction * max_runspeed, accel * delta)
	else:
		velocity.x = move_toward(velocity.x, 0, decel * delta)
	
# When acceleration direction is against current velocity
# May prefer a state to handle this, especially if custom animation
func brake(direction:float, delta: float) -> void:
	velocity.x = move_toward(velocity.x, direction * max_runspeed, braking_decel * delta)

# handle inversions through this function only
func set_inverted(inverted:bool) -> void:
	_invert_multiplier = -1 if inverted else 1
		
# Push this character in a direction, may exceed their max runspeed (but not their max speed)
# Adds to initial velocity
# Consider for external force pushing character
func push(direction: Vector2, pushStrength: float) -> void:
	velocity += direction.normalized() * pushStrength
	velocity.x = clamp(velocity.x, -max_speed, max_speed)
	velocity.y = clamp(velocity.y, -max_speed, max_speed)
