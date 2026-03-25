extends Panel

@onready var hotbarItemVisual: Sprite2D = $CenterContainer/Panel/item_display
@onready var hotbarAmtVisual: Label = $CenterContainer/Panel/amount

# This update function is the same as the inventory's logic. Name conventions are changed to
# associate with hotbar instead of inventory.
func update(slot: InventorySlot):
	if !slot.item:
		hotbarItemVisual.visible = false
		hotbarAmtVisual.visible = false
	else:
		hotbarItemVisual.visible = true
		hotbarItemVisual.texture = slot.item.texture
		if slot.amount > 1: 
			hotbarAmtVisual.visible = true
		hotbarAmtVisual.text = str(slot.amount)
