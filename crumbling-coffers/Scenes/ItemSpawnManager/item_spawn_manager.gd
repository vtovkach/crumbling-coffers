extends Node2D

@export var max_active_items: int = 100
@export var spawn_interval: float = 1
@export var spawn_attempts_per_cycle: int = 60
@export var min_distance_between_items: float = 24.0
@export var ground_collision_mask: int = 2
@export var blocking_collision_mask: int = 4
@export var item_spawn_radius: float = 8.0
@export var support_check_distance: float = 150.0
@export var min_height_above_ground: float = 40.0
@export var max_height_above_ground: float = 140.0
@export var vertical_spawn_bias: float = 120.0

@export var common_weight: int = 70
@export var rare_weight: int = 27
@export var legendary_weight: int = 3

@export var common_items: Array[PackedScene] = []
@export var rare_items: Array[PackedScene] = []
@export var legendary_items: Array[PackedScene] = []

@onready var items_container = get_node_or_null("../Items")

var spawn_timer: float = 0.0

func _ready():
	randomize()
	if items_container == null:
		push_error("ItemSpawnManager: ../Items node not found")

func _process(delta):
	spawn_timer += delta

	if spawn_timer >= spawn_interval:
		spawn_timer = 0.0
		try_spawn_item()

func try_spawn_item():
	if items_container == null:
		return

	if get_active_item_count() >= max_active_items:
		return

	var item_scene = pick_item_scene()
	if item_scene == null:
		return

	for i in range(spawn_attempts_per_cycle):
		var spawn_pos = find_valid_spawn_position()
		if spawn_pos == null:
			continue

		var item_instance = item_scene.instantiate()
		item_instance.global_position = spawn_pos
		items_container.add_child(item_instance)
		return

func get_active_item_count() -> int:
	if items_container == null:
		return 0
	return items_container.get_child_count()

func pick_item_scene() -> PackedScene:
	var total_weight = common_weight + rare_weight + legendary_weight
	if total_weight <= 0:
		return null

	var roll = randi_range(1, total_weight)

	if roll <= common_weight:
		print("ROLL=", roll, " / ", total_weight)
		return pick_random_scene(common_items)
	elif roll <= common_weight + rare_weight:
		print("ROLL=", roll, " / ", total_weight)
		return pick_random_scene(rare_items)
	else:
		print("ROLL=", roll, " / ", total_weight)
		return pick_random_scene(legendary_items)

func pick_random_scene(scene_array: Array[PackedScene]) -> PackedScene:
	if scene_array.is_empty():
		return null
	return scene_array[randi() % scene_array.size()]

func find_valid_spawn_position():
	var spawn_areas = get_tree().get_nodes_in_group("spawn_areas")
	if spawn_areas.is_empty():
		return null

	for i in range(spawn_attempts_per_cycle):
		var area = spawn_areas[randi() % spawn_areas.size()]
		var candidate = area.get_random_point()

		candidate.y -= randf_range(0, vertical_spawn_bias)

		if is_position_valid(candidate):
			return candidate

	return null

func is_position_valid(pos: Vector2) -> bool:
	if is_overlapping_wall(pos):
		return false

	if is_overlapping_item(pos):
		return false

	if not has_support_below(pos):
		return false

	return true

func has_support_below(pos: Vector2) -> bool:
	var space_state = get_world_2d().direct_space_state

	var query = PhysicsRayQueryParameters2D.create(
		pos,
		pos + Vector2(0, max_height_above_ground)
	)
	query.collision_mask = ground_collision_mask

	var result = space_state.intersect_ray(query)
	if result.is_empty():
		return false

	var distance_to_ground = result.position.y - pos.y
	return distance_to_ground >= min_height_above_ground

func is_overlapping_wall(pos: Vector2) -> bool:
	var space_state = get_world_2d().direct_space_state

	var circle = CircleShape2D.new()
	circle.radius = item_spawn_radius

	var params = PhysicsShapeQueryParameters2D.new()
	params.shape = circle
	params.transform = Transform2D(0, pos)
	params.collide_with_bodies = true
	params.collide_with_areas = true
	params.collision_mask = blocking_collision_mask

	var results = space_state.intersect_shape(params)
	return results.size() > 0

func is_overlapping_item(pos: Vector2) -> bool:
	if items_container == null:
		return false

	for item in items_container.get_children():
		if item.global_position.distance_to(pos) < min_distance_between_items:
			return true
	return false
