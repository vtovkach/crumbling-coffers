extends Node2D

@export var max_active_items: int = 10
@export var spawn_interval: float = 3.0
@export var spawn_attempts_per_cycle: int = 20
@export var min_distance_between_items: float = 24.0
@export var ground_collision_mask: int = 2

@export var common_weight: int = 70
@export var rare_weight: int = 25
@export var legendary_weight: int = 5

@export var common_items: Array[PackedScene] = []
@export var rare_items: Array[PackedScene] = []
@export var legendary_items: Array[PackedScene] = []

@onready var items_container = $"../Items"

var spawn_timer: float = 0.0

func _ready():
	randomize()

func _process(delta):
	spawn_timer += delta

	if spawn_timer >= spawn_interval:
		spawn_timer = 0.0
		try_spawn_item()

func try_spawn_item():
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
	return items_container.get_child_count()

func pick_item_scene() -> PackedScene:
	var total_weight = common_weight + rare_weight + legendary_weight
	if total_weight <= 0:
		return null

	var roll = randi_range(1, total_weight)

	if roll <= common_weight:
		return pick_random_scene(common_items)
	elif roll <= common_weight + rare_weight:
		return pick_random_scene(rare_items)
	else:
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
		var floor_pos = get_floor_position(candidate)

		if floor_pos == null:
			continue

		if is_position_valid(floor_pos):
			return floor_pos

	return null

func get_floor_position(start_pos: Vector2):
	var space_state = get_world_2d().direct_space_state

	var query = PhysicsRayQueryParameters2D.create(
		start_pos,
		start_pos + Vector2(0, 200)
	)
	query.collision_mask = ground_collision_mask

	var result = space_state.intersect_ray(query)

	if result.is_empty():
		return null

	return result.position + Vector2(0, -8)

func is_position_valid(pos: Vector2) -> bool:
	if is_overlapping_wall(pos):
		return false

	if is_overlapping_item(pos):
		return false

	return true

func is_overlapping_wall(pos: Vector2) -> bool:
	var space_state = get_world_2d().direct_space_state

	var circle = CircleShape2D.new()
	circle.radius = 8.0

	var params = PhysicsShapeQueryParameters2D.new()
	params.shape = circle
	params.transform = Transform2D(0, pos)
	params.collide_with_bodies = true
	params.collide_with_areas = false
	params.collision_mask = ground_collision_mask

	var results = space_state.intersect_shape(params)
	return results.size() > 0

func is_overlapping_item(pos: Vector2) -> bool:
	for item in items_container.get_children():
		if item.global_position.distance_to(pos) < min_distance_between_items:
			return true
	return false
