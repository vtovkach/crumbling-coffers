extends Panel

# will add future textures that will make use of the variable "bgSprite". its main use would be to switch textures 
# within the specific sprite frame.
#@onready var bgSprite: Sprite2D = $background
@onready var itemVisual: Sprite2D = $CenterContainer/Panel/item_display
@onready var amountVisual: Label = $CenterContainer/Panel/amount

# update function: if there is no item in the inventory_slot, do not show visibility.
# If there is an item, then update and turn on the visibility to true.
func update(slot: InventorySlot):
	# Check if there is a slot inside the array.
	if slot == null:
		itemVisual.visible = false
		amountVisual.visible = false
		return
	
	if slot.item == null:
		itemVisual.visible = false
		amountVisual.visible = false
	else:
		itemVisual.visible = true
		itemVisual.texture = slot.item.texture
		if slot.amount > 1: 
			amountVisual.visible = true
		amountVisual.text = str(slot.amount)
