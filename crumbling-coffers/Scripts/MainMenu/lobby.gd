extends Control

var elapsed_time: int = 0
var searching: bool = false

@onready var hover_search_style = preload("res://Styles/lobby_menu/hover_search_style.tres")
@onready var hover_cancel_style = preload("res://Styles/lobby_menu/hover_cancel_style.tres")

@onready var pressed_search_style = preload("res://Styles/lobby_menu/pressed_search_style.tres")
@onready var pressed_cancel_style = preload("res://Styles/lobby_menu/pressed_cancel_style.tres")

@onready var search_cancel_button = $CenterContainer/MarginContainer2/VBoxContainer/find_match_button
@onready var search_panel = $MarginContainer1/SearchPanelContainer
@onready var timer_label = $MarginContainer1/SearchPanelContainer/SearchTimerLabel
@onready var search_timer = $SearchTimer

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	# Playe music 
	MusicManager.play_music("res://Assets/Music/little-bird.mp3")
	
	search_panel.visible = false
	timer_label.visible = false
	timer_label.text = "00:00"

	search_cancel_button.add_theme_stylebox_override("hover", hover_search_style)
	search_cancel_button.add_theme_stylebox_override("pressed", pressed_search_style)
	
	search_timer.timeout.connect(_on_search_timer_timeout)
	
# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

# =========== BUTTON HANDLERS =============
func _on_find_match_button_pressed() -> void:
	if searching:
		stop_search()
	else:
		start_search()

func _on_cancel_button_pressed() -> void:
	stop_search()
	
func _on_back_button_pressed() -> void:
	get_tree().change_scene_to_file("res://Scenes/Menu/main_menu.tscn")

# ========== SEARCH CONTROL ===========
func start_search() -> void:

	# TODO: send matchmaking request to server via NetworkManager.send_tcp()

	searching = true
	elapsed_time = 0
	
	search_panel.visible = true
	timer_label.visible = true
	timer_label.text = "Searching... 00:00"
	
	search_cancel_button.text = "Cancel"
	search_cancel_button.add_theme_stylebox_override("hover", hover_cancel_style)
	search_cancel_button.add_theme_stylebox_override("pressed", pressed_cancel_style)
	
	search_timer.start()
	
func stop_search() -> void:
	searching = false
	
	search_timer.stop()
	elapsed_time = 0
	
	search_panel.visible = false
	timer_label.visible = false
	timer_label.text = "00:00"
	
	search_cancel_button.text = "Find Match"
	search_cancel_button.add_theme_stylebox_override("hover", hover_search_style)
	search_cancel_button.add_theme_stylebox_override("pressed", pressed_search_style)
	
	# TODO: send cancel matchmaking request to server via NetworkManager.send_tcp()

# TIMER UPDATE
func _on_search_timer_timeout() -> void:
	if not searching:
		return
	elapsed_time += 1
	timer_label.text = "Searching... " + format_time(elapsed_time)

# HELPERS 
func format_time(seconds: int) -> String:
	var minutes = seconds / 60
	var secs = seconds % 60
	return "%02d:%02d" % [minutes, secs]
