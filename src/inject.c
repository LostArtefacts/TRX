#include "inject.h"

#include "specific/ati.h"
#include "specific/input.h"
#include "specific/output.h"
#include "specific/shell.h"
#include "specific/smain.h"
#include "specific/sndpc.h"

void T1MInject()
{
    T1MInjectSpecificATI();
    T1MInjectSpecificInput();
    T1MInjectSpecificOutput();
    T1MInjectSpecificSMain();
    T1MInjectSpecificShell();
    T1MInjectSpecificSndPC();
}
