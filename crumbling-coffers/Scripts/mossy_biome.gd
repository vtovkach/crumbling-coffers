extends BaseLevel

func _ready() -> void:
	# Assign local $Player node to variable inherited from BaseLevel
	player = $Player
	# Call base_level's _ready func
	super._ready()
	# Link player to HUD
	hud.bind_to_player(player)
	hud.set_player_to_indicators(player)
	
	start_training()
	
func start_training() -> void:
	# Pause player movement until countdown stops
	# Disables _physics_process in player.gd
	player.set_physics_process(false)
	# Tell MatchManager to enter COUNTDOWN state.
	MatchManager.start_countdown()
	
# Override BaseLevel function to unpause
func _on_countdown_finished() -> void:
	# Call parent version to show "GO!" on HUD
	super._on_countdown_finished()
	# Specific to mossy biome
	player.set_physics_process(true)
