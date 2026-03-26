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
	_remove_indicators(items)

func _spawn_indicators(items: Array) -> void:
	pass	# TO BE IMPLEMENTED
	
func _remove_indicators(items: Array) -> void:
	var valid_items: Dictionary[PickupBase, bool] = {}

	for item in items:
		valid_items[item] = true

	for item in indicators.keys():
		if not valid_items.has(item):
			_remove_indicator(item)

func _spawn_indicator(item: PickupBase) -> void:
	var indicator = indicator_scene.instantiate() as TwoPointItemIndicator
	add_child(indicator)
	indicator.init(player, item)
	indicators[item] = indicator

func _remove_indicator(item: PickupBase) -> void:
	if not indicators.has(item):
		return
	
	var indicator = indicators[item]
	indicator.destroy()
	indicators.erase(item)

func setPlayer(p: Player) -> void:
	player = p
