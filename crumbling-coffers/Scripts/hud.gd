extends CanvasLayer
class_name HUD

@onready var score_label: Label = $ScoreLabel
# Mapped to countdown and game timer labels in scene
@onready var countdown_label: Label = $CenterContainer/CountdownLabel
@onready var game_timer_label: Label = $MarginContainer/GameTimerLabel

@onready var inventory = $Inv_UI
@onready var hotbar = $hotbar_ui
@onready var item_indicator_manager = $VisualIndicators/ItemIndicators/ItemIndicatorManager

func _ready() -> void:
	# Hide the countdown by default
	hide_countdown()
	# Connect signals that decrement time and signal the match end.
	MatchManager.time_updated.connect(self.update_game_timer)
	MatchManager.match_ended.connect(self._on_match_ended)
	MatchManager.prematch_updated.connect(self._on_prematch_updated)
	MatchManager.prematch_ended.connect(self.hide_countdown)
	MatchManager.match_started.connect(self._on_match_started)
	
	#Have main inventory toggled off by default.
	inventory.close()

func _on_player_score_changed(new_score: int) -> void:
	score_label.text = "Score: %d" % new_score

func bind_to_player(player) -> void:
	player.score_changed.connect(_on_player_score_changed)
	_on_player_score_changed(player.score)

# Updating timer countdown UI.
func update_countdown_text(text: String) -> void:
	countdown_label.show()
	countdown_label.text = text

func hide_countdown() -> void:
	countdown_label.hide()
	
func update_game_timer(seconds: int) -> void:
	game_timer_label.text = str(seconds)

# Check for signal that match is over. Then, will redirect and pause/disable game 
# controls later.
func _on_match_ended() -> void:
	update_countdown_text("MATCH OVER") # Visual indication for match ending
	

# function _input will "listen" for an event when "toggle_inventory" occurs. The button connected is "E".
func _input(event):
	if event.is_action_pressed("toggle_inventory") and MatchManager.current_state == MatchManager.MatchState.RUNNING:
		if inventory.is_open:
			inventory.close()
		else:
			inventory.open()
	
	# Ensure that the hotbar can only be parsed through when the match is in the RUNNING state. No dependencies on 
	# what mode the match is in.
	# Adding in scroll mechanics for the hotbar.
	if MatchManager.current_state == MatchManager.MatchState.RUNNING:
		if event.is_action_pressed("scroll_up"):
			hotbar.active_item_scroll_up()
		elif event.is_action_pressed("scroll_down"):
			hotbar.active_item_scroll_down()
	
	# Hotbar slot can be selected with numbers.
	for i in range(8):
		if MatchManager.current_state == MatchManager.MatchState.RUNNING:
			if event.is_action_pressed("hotbar_slot_%d" %(i+1)):
				hotbar.set_active_slot(i)

func _on_match_started() -> void:
	update_countdown_text("GO!")
	await get_tree().create_timer(2.0).timeout
	hide_countdown()

func _on_prematch_updated(seconds: int) -> void:
	update_countdown_text("Starts In %d" % seconds)

func set_player_to_indicators(p: Player) -> void:
	item_indicator_manager.set_player(p)
