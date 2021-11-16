#include "inject.h"

#include "specific/s_ati.h"
#include "specific/s_main.h"

void T1MInject()
{
    T1MInjectSpecificATI();
    T1MInjectSpecificSMain();
}
