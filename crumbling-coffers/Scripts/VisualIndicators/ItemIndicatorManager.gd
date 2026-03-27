extends Node
class_name ItemIndicatorManager

@export var indicator_scene: PackedScene
var player: Player

var indicators: Dictionary[int, TwoPointItemIndicator] = {}	# int: id of pickup

func _ready() -> void:
	pass
	
func _process(delta: float) -> void:
	if not player: 
		return

	# Yes... every frame get all the pickups. I am aware that this MIGHT NOT be a great idea. 
	# This system integrates a lot of moving and misplaced parts so I'm willing to make a convenient inefficiency
	var items = get_tree().get_nodes_in_group("pickups")
	_update_indicators_list(items)

func _update_indicators_list(items: Array) -> void:
	var valid_ids: Dictionary[int, bool] = {}

	# Spawn
	for item: PickupBase in items:
		if not is_instance_valid(item):
			continue
			
		if _should_have_indicator(item):
			var id: int = item.get_instance_id()
			valid_ids[id] = true
			
			if not indicators.has(id):
				_spawn_indicator(item)
	
	# Remove
	for id: int in indicators.keys():
		if not valid_ids.has(id):
			_remove_indicator(id)

func _spawn_indicator(target: PickupBase) -> void:
	var indicator = indicator_scene.instantiate() as TwoPointItemIndicator
	add_child(indicator)
	indicator.init(player, target)
	indicators[target.get_instance_id()] = indicator

func _remove_indicator(id: int) -> void:
	if not indicators.has(id):
		return
	
	var indicator : TwoPointItemIndicator = indicators[id]

	if is_instance_valid(indicator):
		indicator.queue_free()

	indicators.erase(id)

# This function checks if the item should have indicator. 
func _should_have_indicator(item: PickupBase) -> bool:
	return true # FOR NOW it will be true. 
	

func setPlayer(p: Player) -> void:
	player = p
