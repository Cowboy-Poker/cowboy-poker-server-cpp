#pragma once

#include "Types.h"
#include "CoreMacro.h"
#include "CoreTls.h"
#include "CoreGlobal.h"
#include "Container.h"

#include <chrono>
#include <iostream>
#include <Windows.h>
using namespace std;

#include <winSock2.h>
#include <mswsock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "Lock.h"
#include "Memory.h"
#include "SendBuffer.h"
#include "Session.h"