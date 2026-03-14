extends Node2D

@onready var player = $Player
@onready var hud = $HUD

# Variable to track countdown state
var countdown_number: int = 3

func _ready() -> void:
	hud.bind_to_player(player)
	
	# Start training ground
	start_training()
	
func start_training() -> void:
	# Pause player movement until countdown stops
	# Disables _physics_process in player.gd
	player.set_physics_process(false)
	
	# Run countdown loop
	while countdown_number > 0:
		hud.update_countdown_text(str(countdown_number))
		# Wait 1 second
		await get_tree().create_timer(1.0).timeout
		countdown_number -= 1
		
	# Start message
	hud.update_countdown_text("GO!")
	
	# Short delay for visual legibility
	await get_tree().create_timer(0.5).timeout
	hud.hide_countdown()
	
	# Unpause player
	player.set_physics_process(true)
	
	# Start game timer
	start_game_clock()
	
func start_game_clock() -> void:
	# Start the 60s timer in HUD
	hud.start_game_timer()
