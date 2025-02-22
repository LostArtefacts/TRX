# Commands
TR2X introduces a developer console, by default accessible with the <kbd>/</kbd> key.
Currently supported commands:

- `/pos`  
  Retrieves precise information about Lara's coordinates. Knowledge is power!

- `/tp {room_number}`  
  `/tp {x} {y} {z}`  
  Instant travel! Teleports Lara into a random spot within the specified room or the specified X,Y,Z coordinates.

- `/hp`  
  `/hp {health}`  
  Sets Lara's health points to the specified value. Tougher trials await!

- `/heal`  
  Tough day, Lara? Heals our girl back to full health.

- `/flip`  
  `/flip off`  
  `/flip on`  
  Switches the global flipmap on or off, turning the reality around you on its head.

- `/give {item_name}`  
  `/give {num} {item_name}`  
  `/give all`  
  `/give guns`  
  `/give keys`  
  Gives Lara an item. Try `/give guns` to arm her to the teeth, and `/give keys` to get her all important puzzle items. Ain't nobody got time for searching!

- `/kill`  
  `/kill all`  
  `/kill {enemy_type}`  
  Tired of rats, spiders, tigers, Italian mafia and literally every living thing trying to spoil your adventure? This command instantly disposes of the nearest enemy, or kills them all at once.

- `/fly`  
  Turns on the fly cheat. Why even walk?

- `/cheats on`  
  `/cheats off`  
  Enables or disables cheat keys. It's not like disabling them will make this console any less of a cheat.

- `/wireframe on`  
  `/wireframe off`  
  Enables or disables the wireframe mode. Enter the debugging realm!

- `/endlevel`  
  Ends the current level. Ideal for speedruns.

- `/level {num}`  
  `/level {name}`  
  `/play {num}`  
  `/play {name}`  
  Plays the specified level.

- `/cut {num}`  
  `/cutscene {num}`  
  Plays the specified cutscene.

- `/gym`  
  `/home`  
  Plays Lara's Home, if available. Alias of `/play 0`.

- `/load {slot_num}`  
  Loads the specified savegame slot.

- `/save {slot_num}`  
  Saves the game to the specified savegame slot.

- `/demo`  
  `/demo {num}`  
  Starts the specified demo. If no number is chosen, the demos will cycle.

- `/title`  
  Exits the game to main screen.

- `/exit`  
  Closes the game.

- `/speed`  
- `/speed {num}`  
  Retrieves or sets current game speed.

- `/fps`  
- `/fps {num}`  
  Retrieves or sets current frames per second.

- `/set {option}`  
- `/set {option} {value}`  
  Retrieves or assigns a new value to the given configuration option. Some options need a game re-launch to apply. The option names use `-` rather than `_`.

- `/music {track_id}`  
  Plays a given music track. It uses internal game IDs that for historic reasons don't align with the music folder's file names.

- `/sfx`  
- `/sfx {sound}`  
  Plays a given sound sample.
