extends State
class_name PlayerIdle

@export var player: Player

func physics_update(delta: float) -> void:
	player.move(0, delta)

	if player.jump_pressed:
		transitioned.emit(self, "PlayerJump")
		return
	elif not player.is_on_floor():
		transitioned.emit(self, "PlayerFall")
		return
	elif player.direction != 0:
		transitioned.emit(self, "PlayerRun")
		return
