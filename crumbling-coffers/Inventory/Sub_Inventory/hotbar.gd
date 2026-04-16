extends Resource
class_name Hotbar

# Signal will be used to be sent out and then the changes to update the UI will be made and visible.
signal update
# Signal used to send out when an item is used.
signal item_used(index)

# Same insertion functionality as inventory.gd. Using different name conventions.
@export var hotbar_slots: Array[HotbarSlot]

# When initialized, connect the signal "item_used" to the function "_on_item_used".
func _init():
	item_used.connect(self._on_item_used)

func hotbar_insert(item: HotbarItem):
	var itemSlots = hotbar_slots.filter(func(slot): return slot.hotbar_item == item) 
	if !itemSlots.is_empty():
		itemSlots[0].amount += 1
	else:
		var emptySlots = hotbar_slots.filter(func(slot): return slot.hotbar_item == null)
		if !emptySlots.is_empty():
			emptySlots[0].hotbar_item = item
			emptySlots[0].amount = 1
	# Emit hotbar's update signal.
	update.emit()

# Reset the hotbar to have no items when a new match is started. Similar behavior to inventory's reset_inv().
func reset_hotbar() -> void:
	for hotbar_slot in hotbar_slots:
		hotbar_slot.hotbar_item = null
		hotbar_slot.amount = 0
	update.emit()
	
	
func use_item(index: int, scene_tree: SceneTree) -> void:
	if index < 0 or index >= hotbar_slots.size():
		return

	var slot = hotbar_slots[index]
	if slot == null or slot.hotbar_item == null:
		return

	var item = slot.hotbar_item

	if item.resource_path.ends_with("freeze_staff.tres"):
		apply_freeze(scene_tree)
	elif item.resource_path.ends_with("disorientation_staff.tres"):
		apply_disorientation(scene_tree)
	elif item.resource_path.ends_with("slow_staff.tres"):
		apply_slow(scene_tree)
	else:
		return

	item_used.emit(index)


func _on_item_used(index):
	var slot = hotbar_slots[index]
	# check if the slot's hotbar_item is null, then just return (DO NOTHING).
	if (slot == null) || (slot.hotbar_item == null):
		return
	# Decrement the amount by 1.
	if slot.amount > 1:
		slot.amount -= 1
	else: # if slot item is 0, then update the slot properties to be a brand new, empty slot.
		slot.hotbar_item = null
		slot.amount = 0
	
	update.emit()


func apply_freeze(scene_tree: SceneTree) -> void:
	var targets = scene_tree.get_nodes_in_group("freezable")
	for target in targets:
		if target.is_in_group("player"):
			continue
		if target.has_method("apply_freeze"):
			target.apply_freeze(5.0)

func apply_disorientation(scene_tree: SceneTree) -> void:
	var targets = scene_tree.get_nodes_in_group("disorientable")
	for target in targets:
		if target.is_in_group("player"):
			continue
		if target.has_method("apply_disorientation"):
			target.apply_disorientation(5.0)

func apply_slow(scene_tree: SceneTree) -> void:
	var targets = scene_tree.get_nodes_in_group("slowable")
	for target in targets:
		if target.is_in_group("player"):
			continue
		if target.has_method("apply_slow"):
			target.apply_slow(3.0, 0.4)
