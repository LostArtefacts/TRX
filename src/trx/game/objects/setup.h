#pragma once

void Object_SetupAllObjects(void);

// Adds a setup hook that runs after object setup at each level load. Scripts
// use hooks because object records are rebuilt for each level.
void Object_AddSetupHook(void (*hook)(void));
