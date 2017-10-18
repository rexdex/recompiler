#pragma once

#define NO_MINMAX

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <winsock2.h>
//#include <Windows.h>

#include <vector>
#include <string>
#include <deque>
#include <map>
#include <set>
#include <algorithm>

#include "launcherBase.h"
#include "launcherOutputTTY.h"

namespace runtime
{
	class IDevice;
	class IDeviceCPU;

	class RegisterBank;
	class RegisterBankInfo;
}