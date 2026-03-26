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
	_update_indicators_list

func _update_indicators_list(items: Array) -> void:
	var valid_items: Dictionary[PickupBase, bool] = {}

	# Spawn
	for item: PickupBase in items:
		if _should_have_indicator(item):
			valid_items[item] = true
			if not indicators.has(item):
				_spawn_indicator(item)
	
	# Remove
	for item: PickupBase in indicators.keys():
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
	if is_instance_valid(indicator):
		indicator.queue_free()
	indicators.erase(item)

# This function checks if the item should have indicator. 
func _should_have_indicator(item: PickupBase) -> bool:
	return true # FOR NOW it will be true. 
	

func setPlayer(p: Player) -> void:
	player = p
