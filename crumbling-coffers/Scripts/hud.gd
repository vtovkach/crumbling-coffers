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
	
	
# Match start will be handled in MatchManager.gd.
# Note: "_on_game_timer_timeout()" has been moved to MatchManager.gd.

# function _input will "listen" for an event when "toggle_inventory" occurs. The button connected is "E".
func _input(event):
	if event.is_action_pressed("toggle_inventory"):
		if inventory.is_open:
			inventory.close()
		else:
			inventory.open()
	
	# Adding in scroll mechanics for the hotbar.
	if event.is_action_pressed("scroll_up"):
		hotbar.active_item_scroll_up()
	elif event.is_action_pressed("scroll_down"):
		hotbar.active_item_scroll_down()
	
	# Hotbar slot can be selected with numbers.
	for i in range(8):
		if event.is_action_pressed("hotbar_slot_%d" %(i+1)):
			hotbar.set_active_slot(i)

func set_player_to_indicators(p: Player) -> void:
	item_indicator_manager.setPlayer(p)
