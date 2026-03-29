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
	update.emit()


func _on_item_used(index):
	# !!!
	# PLACE FUNCTIONS TO CHECK WHAT TYPE OF ITEM WAS USED IN ORDER TO APPLY CORRECT ABILITIES/PROPERTIES>
	# var item = hotbar_slots[index].hotbar_item
	# item.apply_effect()
	# ^^ CALL THIS FUNCTION AND FLUSH OUT 
	# !!!
	
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
