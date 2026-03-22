extends State
class_name PlayerJump

@export var player: Player
var jump_released: bool

func enter() -> void:
	player.jump()
	jump_released = false

func physics_update(delta: float) -> void:
	player.move(player.direction, delta)	# could be slower but this refactor aims to to preserve behavior
	player.apply_gravity(delta)
	if player.velocity.y >= 0:
		transitioned.emit(self, "PlayerFall")
		return
	# When player stops holding jump, their vertical speed drops. To a player, "hold jump to jump higher"
	if !jump_released and player.jump_pressed == false:
		jump_released = true
		player.velocity.y *= 0.5
