extends Panel

# will add future textures that will make use of the variable "bgSprite". its main use would be to switch textures 
# within the specific sprite frame.
#@onready var bgSprite: Sprite2D = $background
@onready var itemVisual: Sprite2D = $CenterContainer/Panel/item_display
@onready var amountVisual: Label = $CenterContainer/Panel/amount

# update function: if there is no item in the inventory_slot, do not show visibility.
# If there is an item, then update and turn on the visibility to true.
func update(slot: InventorySlot):
	if !slot.item:
		itemVisual.visible = false
		amountVisual.visible = false
	else:
		itemVisual.visible = true
		itemVisual.texture = slot.item.texture
		amountVisual.visible = true
		amountVisual.text = str(slot.amount)
