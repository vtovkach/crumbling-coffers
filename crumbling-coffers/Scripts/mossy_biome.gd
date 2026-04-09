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
	hud.set_player_to_indicators(player)
	
	# Listen for global match manager to signal end of game
	MatchManager.match_ended.connect(_on_match_ended)
	
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
	
	# REMOVED: start_game_clock bc doesn't need to know time info
	# Instead of calling a function, we emit the handshake
	# Signal based handoff ensures timer is synced among peers
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
	MatchManager.set_state(MatchManager.MatchState.WAITING)
	# Reset player inventory to have no items from previous matches.
	player.inventory.reset_inv()
	player.hotbar.reset_hotbar()
	
# Connect from 'canceled' signal
func _on_quit_confirmation_canceled() -> void:
		get_tree().paused = false

# Function runs when end game signal is heard
func _on_match_ended() -> void:
	
	# Fix to ensure UI remains active
	get_tree().paused = false
	
	if player:
		player.set_physics_process(false) # Disables movement

		# Get data from HUD/Player before moving on
		MatchManager.final_score = int(hud.score_label.text)
		
	# Wait 2 seconds for player to read text
	await get_tree().create_timer(2.0).timeout
		
	# Redirect to score page
	get_tree().change_scene_to_file("res://Scenes/Menu/score_page.tscn")
	print("Redirecting to Score Page...")
	
	
