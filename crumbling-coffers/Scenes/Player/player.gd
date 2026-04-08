# 	Does not have multiplayer support yet;
#	Need to differentiate between client player	
#	and other connected players in same room 

extends CharacterBody2D
class_name Player

# Baseline values. Player has this by default.
const BASE_MAX_SPEED: float = 10000.0
const BASE_MAX_RUNSPEED: float = 1400.0
const BASE_MAX_FALLINGSPEED: float = 5600.0
const BASE_ACCEL: float = 8000.0
const BASE_DECEL: float = 8000.0
const BASE_BRAKING_DECEL: float = 16000.0
const BASE_JUMP_VELOCITY: float = -3600.0
const BASE_GRAVITY_STRENGTH: float = 1.0
const BASE_DASH_COOLDOWN: float = 3 # seconds
const BASE_DASH_STRENGTH: float = 4800.0
const BASE_MIDAIR_SLOWDOWN: float = 0.5
const BASE_AUTOJUMP_WINDOW: float = 0.05 # seconds
const BASE_MIDAIRJUMP_WINDOW: float = 0.05 # seconds

# Values used in calculation. Can increase or decrease with
# temporary status effects, can reset to BASE value on expiration.
@export var max_speed: float = BASE_MAX_SPEED
@export var max_runspeed: float = BASE_MAX_RUNSPEED
@export var max_fallingspeed: float = BASE_MAX_FALLINGSPEED
@export var accel: float = BASE_ACCEL
@export var decel: float = BASE_DECEL
@export var braking_decel: float = BASE_BRAKING_DECEL
@export var jump_velocity: float = BASE_JUMP_VELOCITY
@export var gravity_strength: float = BASE_GRAVITY_STRENGTH
@export var dash_cooldown: float = 0 # seconds
@export var dash_strength: float = BASE_DASH_STRENGTH
@export var midair_slowdown: float = BASE_MIDAIR_SLOWDOWN
@export var autojump_window: float = 0 # seconds
@export var midairjump_window: float = 0 # seconds

var dashing = false		# Maybe this will help with animation control too.

@export var is_frozen: bool = false
@export var freeze_time_left: float = 0.0
const BASE_FROZEN_DECEL: float = 1000.0
@export var frozen_decel: float = BASE_FROZEN_DECEL

@export var is_disoriented: bool = false
@export var disorientation_time_left: float = 0.0

@export var is_slowed: bool = false
@export var slow_time_left: float = 0.0
@export var slow_multiplier: float = 1.0
 


# Values for states
@export var direction: float = 0
@export var jump_pressed: bool = false
@export var jump_tapped: bool = false
@export var down_pressed: bool = false

# DO NOT MODIFY outside of set_inverted(bool)
var _invert_multiplier: int = 1

# Inventory and hotbar variables for "collect" and "hotbar_collect" functions.
@export var inventory: Inventory
@export var itemRes: InventoryItem
@export var hotbar: Hotbar
@export var hotbar_itemRes: HotbarItem

#Daniel - adding a score to the character for when they pick up the items.

signal score_changed(new_score: int)

var score: int = 0

func _ready() -> void:
	# Connect signal to func reset_inv_hotbar so is called when the signal is emitted.
	MatchManager.match_ended.connect(reset_inv_hotbar)
	add_to_group("freezable")
	add_to_group("player")
	add_to_group("disorientable")
	add_to_group("slowable")

func add_score(amount: int) -> void:
	score += amount
	emit_signal("score_changed", score)
# Daniel - ending here

func _physics_process(delta: float) -> void:
	if is_frozen:
		freeze_time_left -= delta
		if freeze_time_left <= 0:
			clear_freeze()
			
	if is_disoriented:
		disorientation_time_left -= delta
		if disorientation_time_left <= 0:
			clear_disorientation()
			
	if is_slowed:
		slow_time_left -= delta
		if slow_time_left <= 0.0:
			clear_slow()
			
			
	if not is_frozen:
	# Get inputs
		direction = _invert_multiplier * Input.get_axis("left", "right")
		jump_pressed = Input.is_action_pressed("jump")
		down_pressed = Input.is_action_pressed("down")		# But, that is best saved for a refactor, and ONLY IF its needed.

		jump_tapped = Input.is_action_just_pressed("jump")	# this jump_tapped is technically unnecessary (only an intermediate for getting autojump_window)
															# in any case, i would like to keep it because this seems useful to track 

		update_timers(delta)

		if jump_tapped:
			autojump_window = BASE_AUTOJUMP_WINDOW

		if dashing: 
			dash_decel(delta)

		if Input.is_action_pressed("dash"):
			dash()
	else:
		# Ignore player input while frozen
		direction = 0
		jump_pressed = false
		down_pressed = false

		# Optional: still reduce dash cooldown while frozen
		dash_cooldown = move_toward(dash_cooldown, 0, delta)

	# Update position
	move_and_slide()

func update_timers(delta: float) -> void:
	autojump_window = move_toward(autojump_window, 0, delta)
	dash_cooldown = move_toward(dash_cooldown, 0, delta)
	
func jump() -> void:
	if midairjump_window:
		velocity.y = jump_velocity
		midairjump_window = 0;
		
func apply_gravity(gravity_multiplier: float, delta: float) -> void:
	velocity.y = move_toward(velocity.y, max_fallingspeed, gravity_multiplier * gravity_strength * get_gravity().y * delta)
	

# Update velocity according to direction of movement
# direction: a float in [-1, 1]
# [-1, 0): 	To left.
# 0: 		Stop 
# (0, 1]:	To right.
func move(direction: float, delta: float, multiplier: float = 1) -> void:
	if is_frozen:
		velocity.x = move_toward(velocity.x, 0, frozen_decel * delta)
		return
		
	
	if direction:
		if (velocity.x > 0 and direction < 0) or (velocity.x < 0 and direction > 0):
			brake(direction, delta, multiplier)
		else:
			velocity.x = move_toward(velocity.x, direction * max_runspeed, multiplier * accel * delta)
	else:
		velocity.x = move_toward(velocity.x, 0, multiplier * decel * delta)
	
# Rapidly increase the speed, but increase the deceleration too.
func dash() -> void:
	if dash_cooldown > 0 or direction == 0:	# no dash
		return
	dash_cooldown = BASE_DASH_COOLDOWN 		# yes dash

	# dash will handle its own decel instead of being a force applied to player
	velocity.x += direction * dash_strength	
	dashing = true	
	
func dash_decel(delta) -> void:
	var vx: float = clamp(velocity.x, -max_runspeed, max_runspeed)
	velocity.x = move_toward(velocity.x, vx, decel * delta)
	if vx == velocity.x:
		dashing = false
	
# When acceleration direction is against current velocity
# May prefer a state to handle this, especially if custom animation
func brake(direction:float, delta: float, multiplier: float = 1) -> void:
	velocity.x = move_toward(velocity.x, direction * max_runspeed, multiplier * braking_decel * delta)

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
	
func apply_freeze(duration: float) -> void:
	is_frozen = true
	freeze_time_left = duration

func clear_freeze() -> void:
	is_frozen = false
	freeze_time_left = 0.0
	
	
func apply_disorientation(duration: float) -> void:
	is_disoriented = true
	disorientation_time_left = duration
	set_inverted(true)
	
func clear_disorientation() -> void:
	is_disoriented = false
	disorientation_time_left = 0.0
	set_inverted(false)
	
func apply_slow(duration: float, multiplier: float) -> void:
	is_slowed = true
	slow_time_left = duration
	slow_multiplier = multiplier
	update_slow_stats()
	
func clear_slow() -> void:
	is_slowed = false
	slow_time_left = 0.0
	slow_multiplier = 1.0
	update_slow_stats()	

func update_slow_stats() -> void:
	if is_slowed:
		max_runspeed = BASE_MAX_RUNSPEED * slow_multiplier
		accel = BASE_ACCEL * slow_multiplier
		decel = BASE_DECEL
		dash_strength = BASE_DASH_STRENGTH * slow_multiplier
	else:
		max_runspeed = BASE_MAX_RUNSPEED
		accel = BASE_ACCEL
		decel = BASE_DECEL
		dash_strength = BASE_DASH_STRENGTH


# Function "collect": When player comes across an item, it will call func  "insert" from "inventory_slot.gd"
# to add the item to the inventory resource (data container). This will update main inventory view to display
# the collected item.
func collect(itemRes):
	inventory.insert(itemRes)

# Consumables will call for this function when Player walks over and collides with the hit/collision box. 
# This functionality is the same as inventory's.
func consumable_collect(hotbar_itemRes):
	hotbar.hotbar_insert(hotbar_itemRes)

# Implementing call reset functions for inventory and hotbar when a match ends.
func reset_inv_hotbar():
	inventory.reset_inv()
	hotbar.reset_hotbar()
