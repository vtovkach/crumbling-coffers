extends Node2D

@onready var player = $Player
@onready var hud = $HUD
@onready var quit_confirm = $QuitConfirmation

# Variable to track countdown state
var countdown_number: int = 3
var total_time: int = 60

func _ready() -> void:
	MusicManager.stop_music()
	hud.bind_to_player(player)
	
	# Start training ground
	start_training()
	
func start_training() -> void:
	# Pause player movement until countdown stops
	# Disables _physics_process in player.gd
	player.set_physics_process(false)
	# Tell MatchManager to enter COUNTDOWN state.
	MatchManager.start_countdown()
	
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
	# start_game_clock()
	
# func start_game_clock() -> void:
	# The "start_game_timer" is now "start_match" in MatchManager.
	# MatchManager.start_match(total_time)
	# Instead of calling a function, we emit the handshake
	MatchManager.match_ready.emit()

func _input(event:InputEvent) -> void:
	# Check for the escape key
	if event.is_action_pressed("ui_cancel"):
		get_tree().paused = true
		quit_confirm.popup_centered()

# Connect from 'confirmed' signal
func _on_quit_confirmation_confirmed() -> void:
	get_tree().paused = false
	get_tree().change_scene_to_file("res://Scenes/Menu/main_menu.tscn")

# Connect from 'canceled' signal
func _on_quit_confirmation_canceled() -> void:
	get_tree().paused = false
