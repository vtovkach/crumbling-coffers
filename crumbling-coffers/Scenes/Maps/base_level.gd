extends Node2D
class_name BaseLevel

# Variables to be filled in by child levels
var hud = null
var quit_confirm = null
var spawn_points = null
var player = null

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	# Manual checks due to quit confirmation not being accessed outside of tutorial
	if has_node("HUD"):
		hud = $HUD
	if has_node("QuitConfirmation"):
		quit_confirm = $QuitConfirmation
	# Check for optional node (not in tutorial)
	if has_node("SpawnPoints"):
		spawn_points = $SpawnPoints
	
	# Ensure music stops
	MusicManager.stop_music()
	if quit_confirm:
		quit_confirm.hide()
	
	# Listen for global signals
	MatchManager.match_ended.connect(_on_match_ended)
	MatchManager.countdown_tick.connect(_on_countdown_tick)
	MatchManager.countdown_finished.connect(_on_countdown_finished)
	
	# Runs code specific to mossy_biome or castle_dungeon
	setup_level()

func setup_level() -> void:
	# Meant to be overridden
	pass
	
func get_random_spawn()-> Vector2:
	if spawn_points and spawn_points.get_child_count() > 0:
		# Pick random child
		var children = spawn_points.get_children()
		var random_index = randi() % children.size()
		# Return global position of marker
		return children[random_index].global_position
	# If no spawn exists
	return Vector2.ZERO

# ================= PAUSE DIALOG ================= #
func _input(event:InputEvent) -> void:
	# Check for the escape key
	if event.is_action_pressed("ui_cancel"):
		# Don't want pause in multiplayer; exit immediately
		if MatchManager.mode == MatchManager.MatchMode.MULTIPLAYER:
			return
		# Do want pause in singleplayer
		if quit_confirm:
			get_tree().paused = true
			quit_confirm.popup_centered()
		else:
			print("Missing QuitConfirmation node!")

# Connect from 'confirmed' signal in 'Quit?' popup
func _on_quit_confirmation_confirmed() -> void:
	# Gave match manager redirection responsibilities
	# Using the global quit
	MatchManager.quit_to_main_menu()
	
# Connect from 'canceled' signal in 'Quit?' popup
func _on_quit_confirmation_canceled() -> void:
		get_tree().paused = false
	
# ================== COUNTDOWN ================== #
func _on_countdown_tick(num: int) -> void:
	# Updates HUD each tick
	hud.update_countdown_text(str(num))

func _on_countdown_finished() -> void:
	# Start message
	hud.update_countdown_text("GO!")
	# Short delay for visual legibility
	await get_tree().create_timer(0.5).timeout
	hud.hide_countdown()
	
	# Allow movement now that game starts
	if player:
		player.set_physics_process(true)
	
	# Tell MatchManager game is live
	MatchManager.match_ready.emit()

# Function runs when end game signal is heard
func _on_match_ended() -> void:
	if player:
		player.set_physics_process(false) # Disables movement

		# Get data from HUD/Player before moving on
		MatchManager.final_score = int(hud.score_label.text)
		
	# Wait 2 seconds for player to read text
	await get_tree().create_timer(2.0).timeout
		
	# Redirect to score page
	MatchManager.go_to_score_page()
