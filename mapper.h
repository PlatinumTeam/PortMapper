//-----------------------------------------------------------------------------
// Copyright (c) 2016 The Platinum Team
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef mapper_h
#define mapper_h

#include <TorqueLib/TGE.h>

extern "C" {
	#include <libnatpmp/natpmp.h>
	#include <miniupnpc/miniupnpc.h>
	#include <miniupnpc/upnpcommands.h>
}


#include <vector>

class Mapper {
private:
	static bool canMapUPnP;
	static bool canMapNatPMP;

	void detectProtocol();
public:
	enum PortProtocol {
		PortProtocolUDP  = 1,
		PortProtocolTCP  = 2,
		PortProtocolBoth = 3
	};

	struct MapData {
		U16 localPort;
		U16 externalPort;
		PortProtocol protocol;
		bool remove;
	};

	Mapper();
	~Mapper();
	void stop();

	bool tryMap(U16 localPort, U16 externalPort, PortProtocol protocol, bool remove);
	static void *_tryMapNatPMP(MapData *data);
	static void *_tryMapUPnP(MapData *data);
	static void _detectNatPMP();
	static void _detectUPnP();
};

#endif
