#pragma once

//#define SINGLE_TEST

#ifdef SINGLE_TEST
#include "include/libarrier/arrier.hpp"
#else
#include "include/libarrier/task.hpp"
#endif

#include <cstdio>
#include <iostream>
