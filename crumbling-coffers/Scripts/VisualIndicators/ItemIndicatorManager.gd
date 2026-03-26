extends Node
class_name ItemIndicatorManager

@export var indicator_scene: PackedScene
var player: Player

var indicators: Dictionary[PickupBase, TwoPointItemIndicator] = {}

func _ready() -> void:
	pass
	
func _process(delta: float) -> void:
	if not player: 
		return

	# Yes... every frame get all the pickups. I am aware that this MIGHT NOT be a great idea. 
	# This system integrates a lot of moving and misplaced parts so I'm willing to make a convenient inefficiency
	var items = get_tree().get_nodes_in_group("pickups")
	

func setPlayer(p: Player) -> void:
	player = p
