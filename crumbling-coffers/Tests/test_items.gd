extends GutTest

var PlayerScene = load("res://Scenes/Player/player.tscn")
var ItemScene = load("res://Scenes/items/Common Item.tscn")

func test_item_removed_on_collision():
	# Setup testing subjects
	var player = PlayerScene.instantiate()
	var item = ItemScene.instantiate()
	
	add_child_autofree(player)
	add_child_autofree(item)
	
	# Overlap player and item to force collision
	var collection_point = Vector2(200, 200)
	player.global_position = collection_point
	item.global_position = collection_point
	
	# Wait a couple physics frames for body_entered to process
	await wait_physics_frames(2)
	
	# Check if item is gone
	# is_instance_valid(item) returns false if freed
	assert_false(is_instance_valid(item), "Item should be freed after player overlap.")
	
	# Check score on player
	assert_true(player.score > 0, "Player score should increase when item is collected.")
	
