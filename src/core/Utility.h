//
// Created by George Huan on 2022/10/2.
//

#ifndef UTILITY_H
#define UTILITY_H

#include "time.h"
#include <stdlib.h>

void CPURandomInit() {
    srand(time(NULL));
}

float GetCPURandom() {
    return (float)rand() / (RAND_MAX + 1.0);
}

#endif //UTILITY_H
