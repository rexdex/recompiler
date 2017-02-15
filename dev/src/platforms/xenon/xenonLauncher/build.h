#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <winsock2.h>
#include <Windows.h>
#include <time.h>
#include <mutex>

#include <vector>
#include <string>
#include <deque>
#include <map>
#include <set>
#include <atomic>

#include "../../../launcher/backend/build.h"
#include "../../../launcher/backend/runtimeSymbols.h"

#pragma warning(disable: 4311 )//: 'type cast' : pointer truncation from 'const uint8 *' to 'uint32'
#pragma warning(disable: 4302 )//: 'type cast' : truncation from 'const uint8 *' to 'uint32'
#pragma warning(disable: 4312 )//: 'type cast' : conversion from 'uint32' to 'const char *' of greater size

#include "xenonUtils.h"
#include "xenonCPU.h"
#include "xenonKernel.h"
#include "xenonPlatform.h"


