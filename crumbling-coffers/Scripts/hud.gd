extends CanvasLayer

@onready var score_label: Label = $ScoreLabel
# Mapped to countdown and game timer labels in scene
@onready var countdown_label: Label = $CenterContainer/CountdownLabel
@onready var game_timer_label: Label = $MarginContainer/GameTimerLabel
# Variable to track seconds
var time_left: int = 60
@onready var inventory = $Inv_UI

func _ready() -> void:
	# Hide the countdown by default
	hide_countdown()
	# Set initial text for game clock
	update_game_timer(time_left)
	#Have main inventory toggled off by default.
	inventory.close()

func _on_player_score_changed(new_score: int) -> void:
	score_label.text = "Score: %d" % new_score

func bind_to_player(player) -> void:
	player.score_changed.connect(_on_player_score_changed)
	_on_player_score_changed(player.score)

func update_countdown_text(text: String) -> void:
	countdown_label.show()
	countdown_label.text = text

func hide_countdown() -> void:
	countdown_label.hide()
	
func start_game_timer() -> void:
	$GameTimer.start()

func update_game_timer(seconds: int) -> void:
	game_timer_label.text = str(seconds)

func _on_game_timer_timeout() -> void:
	if time_left > 0:
		time_left -= 1
		update_game_timer(time_left)
	else:
		# Stop the timer when we hit 0
		$GameTimer.stop()
		print("Game Over!")

func _input(event):
	if event.is_action_pressed("toggle_inventory"):
		print("e pressed")
		if inventory.is_open:
			inventory.close()
		else:
			inventory.open()
