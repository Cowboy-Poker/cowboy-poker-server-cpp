#pragma once

#include "Container.h"
#include "CoreGlobal.h"
#include "CoreMacro.h"
#include "CoreTls.h"
#include "Types.h"

#include <Windows.h>
#include <chrono>
#include <functional>
#include <iostream>
using namespace std;

#include <WS2tcpip.h>
#include <mswsock.h>
#include <winSock2.h>
#pragma comment(lib, "ws2_32.lib")

#include "Lock.h"
#include "Memory.h"
#include "SendBuffer.h"
#include "Session.h"
#include "UdpService.h"
#include "UdpSession.h"