extends BaseLevel

# Inherited variables: hud, quit_confirm, spawn_points, player

func setup_level() -> void:
	# For testing that pause doesn't pop up
	MatchManager.mode = MatchManager.MatchMode.MULTIPLAYER
	
	# In multiplayer the networkmanager should spawn players
	# Just find local player here and link to HUD
	# The following is for testing alone:
	if has_node("UserPlayer"):
		player = $UserPlayer
		
		# Use base_level helper to move player to spawn
		var spawn_pos = get_random_spawn()
		if spawn_pos != Vector2.ZERO:
			player.global_position = spawn_pos
			
		# Tells hud which player to track
		hud.bind_to_player(player)
		hud.set_player_to_indicators(player)
		# Freeze player until game starts
		player.set_physics_process(false)
	MatchManager.start_countdown()
