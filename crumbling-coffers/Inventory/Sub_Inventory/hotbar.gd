extends Resource
class_name Hotbar

# Signal will be used to be sent out and then the changes to update the UI will be made and visible.
signal update
# Signal used to send out when an item is used.
signal item_used(index)

# Same insertion functionality as inventory.gd. Using different name conventions.
@export var hotbar_slots: Array[HotbarSlot]
var EMPTY_SLOT = null

func hotbar_insert(item: HotbarItem):
	var itemSlots = hotbar_slots.filter(func(slot): return slot.hotbar_item == item) 
	if !itemSlots.is_empty():
		itemSlots[0].amount += 1
	else:
		var emptySlots = hotbar_slots.filter(func(slot): return slot.hotbar_item == null)
		if !emptySlots.is_empty():
			emptySlots[0].hotbar_item = item
			emptySlots[0].amount = 1
	
	update.emit()

func _on_item_used(index):
	if hotbar_slots[index].amount > 1:
		hotbar_slots[index].amount -= 1
		update.emit()
	elif hotbar_slots[index].amount == 1:
		# DO NOTHING FOR NOW - NEED TO DEBUG ERROR SO BASE OBJECT IS NOT NULL.
		#hotbar_slots[index] = EMPTY_SLOT
		update.emit()
		
