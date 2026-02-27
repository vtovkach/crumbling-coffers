extends CanvasLayer

@onready var score_label: Label = $ScoreLabel

func _on_player_score_changed(new_score: int) -> void:
	score_label.text = "Score: %d" % new_score

func bind_to_player(player) -> void:
	player.score_changed.connect(_on_player_score_changed)
	_on_player_score_changed(player.score)
