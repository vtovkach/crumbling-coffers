extends Resource
class_name Hotbar

# Signal will be used to be sent out and then the changes to update the UI will be made and visible.
signal update

# Same insertion functionality as inventory.gd. Using different name conventions.
@export var hotbar_slots: Array[InventorySlot]

func hotbar_insert(hotbar_item: InventoryItem):
	var itemSlots = hotbar_slots.filter(func(slot): return slot.item == hotbar_item) 
	if !itemSlots.is_empty():
		itemSlots[0].amount += 1
	else:
		var emptySlots = hotbar_slots.filter(func(slot): return slot.item == null)
		if !emptySlots.is_empty():
			emptySlots[0].item = hotbar_item
			emptySlots[0].amount = 1
	
	update.emit()
