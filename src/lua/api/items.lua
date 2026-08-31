local raw = trxc.items
local api = trx.api

require("trx.math")

local Box = api.class("math.Box")
require("trx.query")

api.module("items", {
  order = 2,
  description = "Module for controlling all moveables.",
})

api.number("items.AnimNum", {
  base = 0,
  description = "The animation's number within the object an item is of.",
})

api.number("items.FrameNum", {
  base = 0,
  description = "The frame's number within the animation it belongs to.",
})

api.number("items.AnimState", {
  base = 0,
  description = "An animation state, as the object's own animations number them. What a state "
    .. "means is the object's business: the numbers of a wolf are not the numbers of a door.",
})

api.number("items.Num", {
  base = 0,
  description = "Item number, matching the numbers level editors show.",
})

api.enum("items.PickupMode", {
  backing = "PICKUP_MODE",
  description = "<!--noref: pickup_mode--> The values the `pickup_mode` item property can take. It selects the animation Lara "
    .. "plays when collecting the item.",
  values = {
    NORMAL = "Picked up off the floor.",
    PLINTH_LOW = "Picked up from a low pedestal.",
    PLINTH_HIGH = "Picked up from a high pedestal.",
    HIDDEN = "Hidden behind an object Lara can reach into.",
    CROWBAR = "Pried off the wall using a crowbar.",
    SARCOPHAGUS = "Hidden inside a sarcophagus.",
    PLINTH_SCION = "Similar to PLINTH_HIGH; invokes Lara's extra animation as in Tomb of Qualopec.",
  },
})

api.enum("items.ScaledSpikesMode", {
  backing = "SCALED_SPIKES_MODE",
  description = "<!--noref: scaled_spikes_mode--> The values the `scaled_spikes_mode` item property can take. It determines how "
    .. "spikes behave when triggered.",
  values = {
    LOOPING = "Spikes will extend, wait a brief period, retract, and then the loop will repeat.",
    EXTENDED = "Spikes will extend and remain as-is indefinitely.",
    ONE_SHOT = "Spikes will extend, wait a brief period, retract, and then stop.",
  },
})

api.enum("items.SwitchMode", {
  backing = "SWITCH_MODE",
  description = "<!--noref: switch_mode--> The values the `switch_mode` item property can take. It selects the animation Lara "
    .. "plays when interacting with the item.",
  values = {
    NORMAL = "A regular/classic wall lever.",
    HIDDEN_REACH = "Lara reaches in to activate.",
    HIDDEN_PICKUP = "Lara reaches in to collect a pickup.",
    SHOVE = "A single-use button that requires a shove to activate.",
  },
})

api.enum("items.TriggerType", {
  backing = "ITEM_TRIGGER_KIND",
  description = "The kind of trigger `trx.items.Item:trigger` fires, matching the trigger types a level editor "
    .. "offers. Most are forward triggers that differ only in what trips them in a level; from a "
    .. "script they behave alike, and `TRIGGER` is the one to reach for.",
  values = {
    TRIGGER = "A plain trigger: sets the code bits and, once they are all set, starts the item.",
    ANTITRIGGER = "Takes the trigger back, clearing the code bits. The item is left running so it "
      .. "can stand itself down, which is how a door animates shut.",
    SWITCH = "Toggles the code bits, so firing it a second time takes the trigger back.",
    HEAVY = "A forward trigger a heavy object trips. A falling block reads this to know it was set "
      .. "off by weight.",
    HEAVY_SWITCH = "A switch a heavy object trips.",
  },
})

api.enum("items.WaterfallSound", {
  backing = "WATERFALL_SOUND",
  description = [[<!--noref: loop_sound--> The values the `loop_sound` item property can take. It selects the
    sound a waterfall loops while it runs.]],
  values = {
    NONE = "The waterfall runs silently.",
    SAND = "A pouring sand loop.",
    WATER = "A running water loop.",
  },
})

-- Item handles are bare userdata. Their metatable is populated by the api.type
-- declaration below, and by nothing else: a member of the C ITEM struct that is
-- not named here is not reachable from a script at all.

local function make_properties(item)
  return setmetatable({}, {
    __index = function(_, key)
      if type(key) ~= "string" then
        return nil
      end
      return item:get_property(key)
    end,
    __newindex = function(_, key, value)
      item:set_property(key, value)
    end,
    __pairs = function()
      local names = item:get_property_names()
      local i = 0
      return function()
        i = i + 1
        local name = names[i]
        if name == nil then
          return nil
        end
        return name, item:get_property(name)
      end
    end,
  })
end

-- on_trigger narrows the global event to this one item. trx.events is reached
-- at call time, so its module need not load before this one.
local function item_hook(event_name)
  return function(item, callback)
    return trx.events[event_name](function(fired, ...)
      if fired == item then
        callback(fired, ...)
      end
    end)
  end
end

local ITEM_LISTENER = {
  type = "events.Listener",
  description = "The attached handler.",
}

-- The item-lifecycle methods share a shape: a callback taking this item, over
-- the matching trx.events hook narrowed to it. Only the wording differs.
local function item_lifecycle_method(event_name, description, examples)
  return {
    params = {
      {
        name = "callback",
        type = "function",
        description = "What to run when it happens to this item.",
        params = {
          {
            name = "item",
            type = "items.Item",
            description = "This item.",
          },
        },
      },
    },
    returns = ITEM_LISTENER,
    description = description,
    examples = examples,
    impl = item_hook(event_name),
  }
end

-- What a trigger carries, which both the hook and the per-item hook hand over.
-- A plain table the engine builds, so its keys are entries it holds rather than
-- accessors.
api.type("items.Trigger", {
  record = true,
  description = "What a trigger carried when it fired.",
  fields = {
    type = {
      type = "items.TriggerType",
      description = "The kind of trigger it was.",
    },
    mask = {
      type = "integer",
      description = "The code bits it set, `1` to `31`.",
    },
    timer = {
      type = "game.Seconds",
      description = "How long it keeps the item going.",
    },
    one_shot = {
      type = "boolean",
      description = "Whether it fires only the once.",
    },
  },
})

api.type("items.Item", {
  backing = "ITEM",
  description = "An item, also known as a moveable.",

  fields = {
    pos = {
      from = "pos",
      type = "math.Vec3",
      description = "World position. Updating this also updates `trx.items.Item.room` and `trx.items.Item.room_num`.",
    },
    rot = {
      from = "rot",
      type = "math.Rot",
      description = "Orientation.",
    },
    anim_num = {
      from = "relative_anim_num",
      type = "items.AnimNum",
    },
    frame_num = {
      from = "relative_frame_num",
      type = "items.FrameNum",
      description = "Negative values count back from the end.",
    },
    num = {
      from = "item_num",
      type = "items.Num",
      writable = false,
      description = "An item handed over by a query can say where it lives.",
    },
    room_num = {
      from = "room_num",
      type = "rooms.Num",
      writable = false,
      description = "The room containing this item. Set `trx.items.Item.pos` to move the item between rooms.",
    },
    hit_points = {
      from = "hit_points",
      type = "integer",
      description = "Current hit points. Raising this above the maximum also raises the `max_hit_points` entry of `trx.items.Item.properties`. <!--noref: max_hit_points-->",
    },
    max_hit_points = {
      from = "max_hit_points",
      type = "integer",
      writable = false,
      description = "Maximum hit points. Set the `max_hit_points` entry of `trx.items.Item.properties` to change it. <!--noref: max_hit_points-->",
    },
    name = {
      from = "name",
      type = "string",
      description = "Unique item name, or `nil`. Assigning a name already in use raises an error.",
    },
    object_id = {
      from = "object_id",
      type = "catalog.objects",
      writable = false,
      description = "The item's object type.",
    },
    is_visible = {
      from = "is_visible",
      type = "boolean",
      description = "Whether the item is drawn. It can be present in the world but not visible, "
        .. "like an ambush enemy waiting to appear.",
    },
    is_finished = {
      from = "is_finished",
      type = "boolean",
      description = "Whether the item has finished its run - a creature that died, or a one-shot "
        .. "trigger that fired. It stays in the level but no longer acts.",
    },
    is_present = {
      from = "is_present",
      type = "boolean",
      writable = false,
      description = "Whether the item is in the world at all: linked in its room, so drawn and "
        .. "collidable in principle. Managed by the engine.",
    },
    timer = {
      from = "timer",
      type = "game.Frames",
      description = "How long the item's trigger keeps it going. `0` runs it until something takes "
        .. "the trigger back; `-1` means it has run out; anything else counts down. "
        .. "`trx.items.Item:trigger` takes its own timer as a `trx.game.Seconds`.",
    },
    is_triggered = {
      from = "is_triggered",
      type = "boolean",
      writable = false,
      description = "Whether the item's trigger currently says go. This is what a door, a switch or "
        .. "an alarm reads to decide whether to act; a creature ignores it and goes by whether it "
        .. "is running.\n\n"
        .. "It is a verdict on `trx.items.Item.trigger_mask`, `trx.items.Item.timer` and `trx.items.Item.is_reversed` together, not a field of "
        .. "its own.",
    },
    trigger_mask = {
      from = "trigger_mask",
      type = "integer",
      description = "The five code bits, counted the way a level editor counts them: `1` to `31`. "
        .. "The trigger only says go once every bit is set, which is how a level makes several "
        .. "triggers agree before anything happens. A lone trigger carries all of them.",
    },
    is_reversed = {
      from = "is_reversed",
      type = "boolean",
      description = "Whether the item's trigger is inverted, so it runs until triggered rather than "
        .. "once triggered. This is how a level ships something already on.",
    },
    speed = {
      from = "speed",
      type = "integer",
      description = "Forward speed.",
    },
    fall_speed = {
      from = "fall_speed",
      type = "integer",
      description = "Vertical speed.",
    },
    gravity = {
      from = "gravity",
      type = "boolean",
      description = "Whether gravity applies to this item.",
    },
    collidable = {
      from = "is_collidable",
      type = "boolean",
      description = "Whether Lara can collide with this item.",
    },
    is_alive = {
      from = "is_alive",
      type = "boolean",
      writable = false,
      description = "Whether the item is a living creature with hit points remaining.",
    },
    is_targetable = {
      from = "is_targetable",
      type = "boolean",
      writable = false,
      description = "Whether Lara's auto-aim can lock onto the item right now.",
    },
    is_killed = {
      from = "is_killed",
      type = "boolean",
      writable = false,
      description = "Whether the item has already been killed.",
    },
    is_one_shot = {
      from = "is_one_shot",
      type = "boolean",
      description = "Whether the item's trigger has been spent and will never fire again.",
    },
    is_hostile = {
      from = "is_hostile",
      type = "boolean",
      writable = false,
      description = "Whether this item is a creature currently hostile to Lara.",
    },
    is_ally = {
      from = "is_ally",
      type = "boolean",
      writable = false,
      description = "Whether this item is a creature that fights on Lara's side. An ally is "
        .. "shown in its own colour where an enemy would be.",
    },
    is_simulated = {
      from = "is_simulated",
      type = "boolean",
      writable = false,
      description = "Whether the item's control routine runs each frame. Call `trx.items.Item:activate` to start it.",
    },
    is_in_play = {
      from = "is_in_play",
      type = "boolean",
      writable = false,
      description = "Whether the item is live: simulated, visible and not finished - the state a "
        .. "targetable enemy is in. A read-only composite of the axes.",
    },
    was_hit = {
      from = "hit_status",
      type = "boolean",
      writable = false,
      description = "Whether the item was hit during the current frame.",
    },
    mesh_bits = {
      from = "mesh_bits",
      type = "integer",
      description = "Bitmask of which of the item's meshes are drawn.",
    },
    touch_bits = {
      from = "touch_bits",
      type = "integer",
      writable = false,
      description = "Bitmask of which of the item's meshes Lara is touching.",
    },
    anim_state = {
      from = "current_anim_state",
      type = "items.AnimState",
      description = "The state the item is in.",
    },
    goal_anim_state = {
      from = "goal_anim_state",
      type = "items.AnimState",
      description = "The state the item is transitioning towards.",
    },
    -- Deliberately not exposed: box_num, floor, next_item, next_simulated, gen,
    -- anim_num, frame_num, prev_frame_num, ai_bits, ai_tag, after_death and the
    -- render flags. They are engine internals, not a contract.
  },

  extensions = {
    room = {
      type = "rooms.Room",
      description = "The room containing this item.",
      impl = function(item)
        return trx.rooms[item.room_num]
      end,
    },

    bounds = {
      type = "math.Box",
      description = "The item's bounding box for the frame it is on. The numbers are in the "
        .. "item's own frame, so they say how far the model reaches around "
        .. "`trx.items.Item.pos` before `trx.items.Item.rot` turns it, and they change as the "
        .. "item animates.",
      impl = function(item)
        return setmetatable(raw.get_bounds(item), Box)
      end,
    },

    properties = {
      type = "table",
      description = "Typed, object-specific item properties. Writing here overrides the object's "
        .. "default for this item only; reads fall back to the object. Iterable with `pairs()`. "
        .. "See [Objects](docs/trx/OBJECTS.md).",
      impl = make_properties,
    },
  },

  methods = {
    activate = {
      description = "Brings the item to life, exactly as tripping a trigger on it would: its control "
        .. "routine starts running, and a creature also gets its AI, without which it would stand "
        .. "there and ignore Lara.\n\n"
        .. "Objects with no control routine cannot be activated, and an item that is already active "
        .. "is left alone.",
    },

    deactivate = {
      description = "Stops the item: its control routine no longer runs, and a creature loses its AI "
        .. "and stands down. The item stays where it is and keeps its hit points, so this is not a "
        .. "way of getting rid of it - use `trx.items.Item:destroy` for that.\n\n"
        .. "A trigger can still bring it back, and so can `trx.items.Item:activate`.",
    },

    trigger = {
      description = "Fires a trigger at the item, exactly as a floor trigger in the level would: "
        .. "sets the code bits, and once they are all set, starts the item running.\n\n"
        .. "This is the one to reach for on anything a level would trigger - a door, a switch, an "
        .. "alarm - because those read their trigger before they act, and merely activating "
        .. "one leaves it running but doing nothing. Pass `type = "
        .. "trx.items.TriggerType.ANTITRIGGER` to take the trigger back instead.",
      params = {
        {
          name = "opts",
          type = "table",
          optional = true,
          description = "What the trigger carries.",
          fields = {
            {
              name = "type",
              type = "items.TriggerType",
              optional = true,
              description = "A plain `TRIGGER` by default.",
            },
            {
              name = "mask",
              type = "integer",
              optional = true,
              description = "Which of the five code bits to set, `1` to `31`, all of them by "
                .. "default. Pass fewer to act as one of several triggers a puzzle is waiting "
                .. "on.",
            },
            {
              name = "timer",
              type = "game.Seconds",
              optional = true,
              default = 0,
              description = "How long it should keep the item going. `0` means until something "
                .. "takes the trigger back. A timer of exactly `1` is a single frame, not a "
                .. "second, matching the level format.",
            },
            {
              name = "one_shot",
              type = "boolean",
              optional = true,
              description = "Never let it fire again.",
            },
          },
        },
      },
      examples = {
        [[trx.items[12]:trigger()]],
        [[trx.items[12]:trigger({ timer = 3, one_shot = true })]],
        [[trx.items[12]:trigger({ type = trx.items.TriggerType.ANTITRIGGER })]],
      },
    },

    on_trigger = {
      params = {
        {
          name = "callback",
          type = "function",
          description = "What to run when it happens to this item.",
          params = {
            {
              name = "item",
              type = "items.Item",
              description = "This item.",
            },
            {
              name = "trigger",
              type = "items.Trigger",
              description = "What the trigger carried.",
            },
          },
        },
      },
      returns = ITEM_LISTENER,
      description = "Happens every time a trigger is aimed at this item, of any kind. "
        .. "`trx.events.on_trigger`, narrowed to this item.",
      examples = {
        [[trx.items[12]:on_trigger(function(item, trigger)
  trx.log.info("triggered with mask " .. trigger.mask)
end)]],
      },
      impl = item_hook("on_trigger"),
    },

    on_hit = {
      params = {
        {
          name = "callback",
          type = "function",
          description = "What to run when it happens to this item.",
          params = {
            {
              name = "item",
              type = "items.Item",
              description = "This item.",
            },
            {
              name = "damage",
              type = "integer",
              description = "Hit points taken, before clamping to zero.",
            },
          },
        },
      },
      returns = ITEM_LISTENER,
      description = "Happens when this item takes damage. `trx.events.on_hit`, narrowed to this "
        .. "item.",
      examples = {
        [[trx.items[12]:on_hit(function(item, damage)
  trx.log.info("the item lost " .. damage .. " hit points")
end)]],
      },
      impl = item_hook("on_hit"),
    },

    on_kill = {
      params = {
        {
          name = "callback",
          type = "function",
          description = "What to run when it happens to this item.",
          params = {
            {
              name = "item",
              type = "items.Item",
              description = "This item.",
            },
          },
        },
      },
      returns = ITEM_LISTENER,
      description = "Happens when damage takes this item's hit points to zero. "
        .. "`trx.events.on_kill`, narrowed to this item.",
      examples = {
        [[trx.items[12]:on_kill(function(item)
  trx.log.info("the item is down")
end)]],
      },
      impl = item_hook("on_kill"),
    },

    on_show = item_lifecycle_method(
      "on_show",
      "Happens when this item becomes visible during play. "
        .. "`trx.events.on_show`, narrowed to this item.",
      {
        [[trx.items[12]:on_show(function(item)
  trx.log.info("the item appeared")
end)]],
      }
    ),

    on_hide = item_lifecycle_method(
      "on_hide",
      "Happens when this item becomes hidden during play. "
        .. "`trx.events.on_hide`, narrowed to this item.",
      {
        [[trx.items[12]:on_hide(function(item)
  trx.log.info("the item vanished")
end)]],
      }
    ),

    on_finish = item_lifecycle_method(
      "on_finish",
      "Happens when this item finishes its run during play. "
        .. "`trx.events.on_finish`, narrowed to this item.",
      {
        [[trx.items[12]:on_finish(function(item)
  trx.log.info("the item finished its run")
end)]],
      }
    ),

    on_enter_sim = item_lifecycle_method(
      "on_enter_sim",
      "Happens when this item starts being simulated during play. "
        .. "`trx.events.on_enter_sim`, narrowed to this item.",
      {
        [[trx.items[12]:on_enter_sim(function(item)
  trx.log.info("the item started running")
end)]],
      }
    ),

    on_leave_sim = item_lifecycle_method(
      "on_leave_sim",
      "Happens when this item stops being simulated during play. "
        .. "`trx.events.on_leave_sim`, narrowed to this item.",
      {
        [[trx.items[12]:on_leave_sim(function(item)
  trx.log.info("the item stopped running")
end)]],
      }
    ),

    on_activate = item_lifecycle_method(
      "on_activate",
      "Happens when this item is activated through the lifecycle front door during play. "
        .. "`trx.events.on_activate`, narrowed to this item.",
      {
        [[trx.items[12]:on_activate(function(item)
  trx.log.info("the item was activated")
end)]],
      }
    ),

    on_deactivate = item_lifecycle_method(
      "on_deactivate",
      "Happens when this item is deactivated through the lifecycle front door during play. "
        .. "`trx.events.on_deactivate`, narrowed to this item.",
      {
        [[trx.items[12]:on_deactivate(function(item)
  trx.log.info("the item was deactivated")
end)]],
      }
    ),

    on_destroy = item_lifecycle_method(
      "on_destroy",
      "Happens as this item is removed from the game during play. It can still be read from the "
        .. "handler, but not after. `trx.events.on_destroy`, narrowed to this item.",
      {
        [[trx.items[12]:on_destroy(function(item)
  trx.log.info("the item was removed")
end)]],
      }
    ),

    on_enter_world = item_lifecycle_method(
      "on_enter_world",
      "Happens when this item enters the world during play, such as a runtime spawn. "
        .. "`trx.events.on_enter_world`, narrowed to this item.",
      {
        [[trx.items[12]:on_enter_world(function(item)
  trx.log.info("the item entered the world")
end)]],
      }
    ),

    on_leave_world = item_lifecycle_method(
      "on_leave_world",
      "Happens when this item leaves the world during play. "
        .. "`trx.events.on_leave_world`, narrowed to this item.",
      {
        [[trx.items[12]:on_leave_world(function(item)
  trx.log.info("the item left the world")
end)]],
      }
    ),

    destroy = {
      description = "Removes the item from the game. Any other handle to it becomes stale.",
    },

    is_valid = {
      returns = {
        type = "boolean",
        description = "False once the item it named is gone.",
      },
      description = "Whether the handle still refers to a live item. Reading or writing a field on a "
        .. "stale handle raises an error rather than silently operating on an unrelated item, so "
        .. "check this for a handle held across time.",
      examples = {
        [[local wolf = trx.items.query:of_object(trx.catalog.objects.wolf):first()
trx.events.after_control(function()
  if wolf:is_valid() and wolf.hit_points <= 0 then
    trx.log.info("the wolf is down")
  end
end)]],
      },
    },

    die = {
      params = {
        {
          name = "explode",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether to burst the meshes as it dies.",
        },
      },
      description = "Runs the object's creature death handling: the corpse stays, and `trx.items.Item.die.explode` "
        .. "bursts its meshes as a rocket or grenade would. For creatures; `trx.items.Item:destroy` simply removes "
        .. "any item from the game.",
    },

    take_damage = {
      params = {
        {
          name = "damage",
          type = "integer",
          description = "Hit points to take.",
        },
      },
      description = [[
        Hurts the item the way a weapon does, and reports through
        `trx.events.on_hit`, and `trx.events.on_kill` where the blow takes the
        last hit point. Writing `trx.items.Item.hit_points` reports neither.
        The kill counts as the environment's rather than Lara's.
      ]],
      examples = {
        [[local lara = trx.lara.item
lara:take_damage(lara.hit_points)]],
      },
    },

    shatter = {
      params = {
        {
          name = "damage",
          type = "integer",
          optional = true,
          default = 0,
          description = "Splash damage dealt to nearby items.",
        },
      },
      description = "Bursts the item's meshes into flying debris, the visual `trx.items.Item:die` produces with "
        .. "`trx.items.Item.die.explode`, "
        .. "on its own. It does not kill or remove the item.",
    },

    distance_to = {
      params = {
        { name = "pos", type = "math.Vec3", description = "World position." },
      },
      returns = {
        type = "math.Distance",
        description = "Measured between the two positions.",
      },
      description = "Distance from this item to a world position.",
    },

    get_property = {
      params = {
        {
          name = "name",
          type = "string",
          description = "Which property, as the object declares it.",
        },
      },
      returns = {
        type = "any",
        nullable = true,
        description = "The value, of the type the property is declared with.",
      },
      description = "Reads an object property, falling back to the object's default. "
        .. "Prefer `item.properties.<name>`.",
    },

    set_property = {
      params = {
        {
          name = "name",
          type = "string",
          description = "Which property, as the object declares it.",
        },
        {
          name = "value",
          type = "any",
          description = "What to write, of the type the property is declared with.",
        },
      },
      description = "Overrides an object property for this item. Prefer `item.properties.<name> = ...`.",
    },

    get_property_names = {
      returns = {
        type = "string",
        list = true,
      },
      description = "Names of every property this item's object declares.",
    },
  },
})

api.define("items.get", {
  description = "Retrieves an item by number or by name.",
  params = {
    {
      name = "key",
      type = "items.Num",
      description = "An item's unique name reaches it as well.",
    },
  },
  returns = {
    type = "items.Item",
    nullable = true,
    description = "The item, or `nil` where nothing answers to the key.",
  },
  examples = {
    [==[local item = trx.items[0]
item.name = "lara"
local lara = trx.items["lara"]]==],
  },
  impl = raw.get,
})

api.define("items.spawn", {
  description = "Creates a new item of the given object type at the given position.",
  params = {
    {
      name = "object_id",
      type = "catalog.objects",
      description = "Object type to spawn.",
    },
    {
      name = "pos",
      type = "math.Vec3",
      description = "World position. Must lie inside the level.",
    },
    {
      name = "angle_y",
      type = "math.Angle",
      optional = true,
      default = 0,
      description = "Facing angle.",
    },
    {
      name = "opts",
      type = "table",
      optional = true,
      description = "How to spawn it.",
      fields = {
        {
          name = "activate",
          type = "boolean",
          optional = true,
          description = "Bring the item to life, enabling AI for creatures.",
        },
      },
    },
  },
  returns = {
    type = "items.Item",
    nullable = true,
    description = "`nil` if the item pool is exhausted.",
  },
  examples = {
    [[local wolf = trx.items.spawn(
  trx.catalog.objects.wolf, trx.lara.item.pos, 0, { activate = true })]],
  },
  impl = raw.spawn,
})

api.define("items.count", {
  description = "Returns the total number of allocated items. Same as `#trx.items`.",
  returns = {
    type = "integer",
    description = "How many slots the level holds, live or not.",
  },
  impl = raw.count,
})

-- Every item the level holds, each by its number.
local function enumerate()
  local out = {}
  for i = 0, raw.count() - 1 do
    local item = raw.get(i)
    if item ~= nil then
      out[#out + 1] = { i, item }
    end
  end
  return out
end

-- An object named by string resolves through the object query, so `of_object`
-- takes a name the same way a player would. An id passes straight through.
-- trx.objects is reached at call time, not required: it is up long before a
-- console line runs, and leaving it out keeps a script that only queries items
-- from dragging the object surface in behind it.
local function resolve_object(key)
  if type(key) == "number" then
    return key
  end
  return trx.objects.query:by_name(key):ids()[1]
end

-- A spatial answer as a predicate: the numbers come back as a list, and a
-- query asks after one item at a time.
local function found_in(nums)
  local set = {}
  for _, num in ipairs(nums) do
    set[num] = true
  end
  return function(num)
    return set[num] == true
  end
end

-- One of an item's own true-or-false axes, as a narrowing.
local function axis_narrowing(field, description)
  return {
    description = description,
    returns = { type = "query.Query", description = "The narrowed query." },
    impl = trx.query.narrowing(function()
      return function(_i, item)
        return item[field]
      end
    end),
  }
end

local ItemQuery = api.type("items.ItemQuery", {
  extends = "query.Query",
  description = "A `trx.query.Query` over the items a level holds, with the narrowings below on top "
    .. "of the ones every query has. Items answer to no names of their own, so `trx.items.ItemQuery:of_object` is how a "
    .. "name reaches them.",

  methods = {
    simulated = axis_narrowing(
      "is_simulated",
      "The item is being simulated: its control routine runs every frame."
    ),
    present = axis_narrowing(
      "is_present",
      "The item is in the world, whether or not anything is simulating it."
    ),
    visible = axis_narrowing("is_visible", "The item is drawn."),
    finished = axis_narrowing("is_finished", "The item has run its course."),
    in_play = axis_narrowing(
      "is_in_play",
      "The item is part of the game rather than set aside."
    ),
    alive = axis_narrowing("is_alive", "The item still has hit points."),
    targetable = axis_narrowing(
      "is_targetable",
      "Lara's guns can lock onto the item."
    ),

    of_object = {
      description = "The item is of the given object, named the way a player would name it or by "
        .. "its id.",
      params = {
        {
          name = "key",
          type = "any",
          description = "Object id, or a name `trx.objects.query` resolves.",
        },
      },
      returns = { type = "query.Query", description = "The narrowed query." },
      examples = {
        [[trx.items.query:of_object("wolf"):simulated():matches()]],
      },
      impl = trx.query.narrowing(function(key)
        local object_id = resolve_object(key)
        return function(_i, item)
          return object_id ~= nil and item.object_id == object_id
        end
      end),
    },

    in_room = {
      description = "The item is in the given room.",
      params = {
        {
          name = "room_num",
          type = "rooms.Num",
        },
      },
      returns = { type = "query.Query", description = "The narrowed query." },
      impl = trx.query.narrowing(function(room_num)
        return function(_i, item)
          return item.room_num == room_num
        end
      end),
    },

    in_box = {
      description = "The item stands inside a world-space box. The corners may come in any order.\n\n"
        .. "An item is tested by its position, the point it stands at, rather than by the box it "
        .. "fills. Position is all this asks after, so the rest of the query says what else the "
        .. "item must be: `trx.items.query:in_box(min, max):present()` asks for the ones that are "
        .. "in the world as well.",
      params = {
        {
          name = "min",
          type = "math.Vec3",
          description = "One corner of the box.",
        },
        {
          name = "max",
          type = "math.Vec3",
          description = "The opposite corner.",
        },
      },
      returns = { type = "query.Query", description = "The narrowed query." },
      examples = {
        [[local guards = trx.items.query
  :in_box({ x = 51200, y = -2048, z = 30720 }, { x = 53248, y = 0, z = 32768 })
  :present()
  :matches()]],
      },
      impl = trx.query.narrowing(function(min, max)
        return found_in(raw.in_box(min, max))
      end),
    },

    in_sphere = {
      description = "The item stands within a radius of a point. As with `trx.items.ItemQuery:in_box`, the item's "
        .. "position is the whole of the test.",
      params = {
        {
          name = "centre",
          type = "math.Vec3",
          description = "Middle of the sphere.",
        },
        {
          name = "radius",
          type = "math.Distance",
          description = "How far out it reaches.",
        },
      },
      returns = { type = "query.Query", description = "The narrowed query." },
      impl = trx.query.narrowing(function(centre, radius)
        return found_in(raw.in_sphere(centre, radius))
      end),
    },
  },
})

local item_query = trx.query.new({
  enumerate = enumerate,
  id_of = function(i)
    return i
  end,
}, ItemQuery)

api.property("items.query", {
  type = "items.ItemQuery",
  description = "The identity query over every item in the level. Narrow it and read it.",
  get = function()
    return item_query
  end,
})

api.container("items", {
  description = "Indexing the module reaches an item, and `#trx.items` is how many the level has. "
    .. "`pairs()` walks them in order, keyed by the item number.",
  key = {
    type = { "items.Num", "string" },
    description = "An item's unique name reaches it as well.",
  },
  value = { type = "items.Item", nullable = true },
  examples = {
    [[for num, item in pairs(trx.items) do
  trx.log.info(item.object_id)
end]],
  },
  get = raw.get,
  count = raw.count,
})
