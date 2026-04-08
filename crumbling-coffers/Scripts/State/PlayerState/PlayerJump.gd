extends State
class_name PlayerJump

@export var player: Player
var jump_released: bool
const jump_slowdown_multiplier: float = 0.5	

func enter() -> void:
	player.jump()
	jump_released = false

func physics_update(delta: float) -> void:
	player.move(player.direction, delta, player.midair_slowdown)	# could be slower but this refactor aims to to preserve behavior
	player.apply_gravity(1, delta)
	if player.velocity.y >= 0:
		transitioned.emit(self, "PlayerFall")
		return
	if player.down_pressed == true:
		transitioned.emit(self, "PlayerFall")
		
	# When player stops holding jump, their vertical speed drops. To a player, "hold jump to jump higher"
	if !jump_released and player.jump_pressed == false:
		jump_released = true
		player.velocity.y *= jump_slowdown_multiplier
