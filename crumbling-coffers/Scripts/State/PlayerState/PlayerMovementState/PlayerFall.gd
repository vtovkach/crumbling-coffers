extends PlayerState
class_name PlayerFall

func physics_update(delta: float) -> void:
	player.move(player.direction, delta, player.midair_slowdown)	# could be slower but this refactor aims to to preserve behavior
	if player.midairjump_window and player.autojump_window:
		transitioned.emit(self, "PlayerJump")
		return
		
	player.midairjump_window = move_toward(player.midairjump_window, 0, delta)
	
	player.apply_gravity(2 if player.down_pressed else 1, delta)
	if player.is_on_floor():
		transitioned.emit(self, "PlayerRun" if player.direction != 0 else "PlayerIdle")
		return
