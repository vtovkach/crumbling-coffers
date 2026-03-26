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
	_spawn_indicators(items)
	_destroy_indicators(items)

func _spawn_indicators(items: Array) -> void:
	pass	# NOT IMPLEMENTED
	
func _destroy_indicators(items: Array) -> void:
	var valid_items: Dictionary[PickupBase, bool] = {}

	for item in items:
		valid_items[item] = true

	for item in indicators.keys():
		if not valid_items.has(item):
			_remove_indicator(item)

func _remove_indicator(item: PickupBase) -> void:
	pass	# NOT IMPLEMENTED

func setPlayer(p: Player) -> void:
	player = p
