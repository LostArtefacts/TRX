#include "inject.h"

#include "specific/ati.h"
#include "specific/smain.h"

void T1MInject()
{
    T1MInjectSpecificATI();
    T1MInjectSpecificSMain();
}
