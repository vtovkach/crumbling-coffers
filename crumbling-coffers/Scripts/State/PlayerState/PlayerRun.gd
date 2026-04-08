extends State
class_name PlayerRun

@export var player: Player

func enter() -> void:
	player.midairjump_window = player.BASE_MIDAIRJUMP_WINDOW

func physics_update(delta: float) -> void:
	player.move(player.direction, delta)

	if player.autojump_window:
		transitioned.emit(self, "PlayerJump")
		return
	elif not player.is_on_floor():
		transitioned.emit(self, "PlayerFall")
		return
	elif player.direction == 0:
		transitioned.emit(self, "PlayerIdle")
		return
