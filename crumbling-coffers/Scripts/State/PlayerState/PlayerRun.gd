extends State
class_name PlayerRun

@export var player: Player

func physics_update(delta: float) -> void:
	player.move(player.direction, delta)

	if player.jump_pressed:
		transitioned.emit(self, "PlayerJump")
		return
	elif not player.is_on_floor():
		transitioned.emit(self, "PlayerFall")
		return
