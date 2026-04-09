extends PlayerState
class_name PlayerIdle

func enter() -> void:
	super()
	player.midairjump_window = player.BASE_MIDAIRJUMP_WINDOW

func physics_update(delta: float) -> void:
	player.move(0, delta)

	if player.autojump_window:
		transitioned.emit(self, "PlayerJump")
		return
	elif not player.is_on_floor():
		transitioned.emit(self, "PlayerFall")
		return
	elif player.direction != 0:
		transitioned.emit(self, "PlayerRun")
		return
