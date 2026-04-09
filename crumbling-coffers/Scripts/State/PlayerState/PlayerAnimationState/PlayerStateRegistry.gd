extends Object
class_name PlayerStateRegistery

# Simply, store a constant mapping of state names to state ids
# So that state can be consistently translated on the way to server and back

# client state -> int
const PLAYERSTATE_MAP = {
#	Cannot be completed at this time. 
#	This will be for *animation states* which do not exist yet
}

# String: name of state
static func encode(state: String) -> int:
	return PLAYERSTATE_MAP.get(state, 0)

static func decode(value: int) -> String:
	return PLAYERSTATE_MAP.find_key(value)
