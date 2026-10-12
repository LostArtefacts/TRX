#pragma once

// Deletes files that an earlier release shipped and the current one does not,
// as listed in cfg/tombstones.json5. A file that differs from every shipped
// version stays in place, and the log names it.
void Shell_SweepTombstones(void);
