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

#include <PluginLoader/PluginInterface.h>
#include <TorqueLib/TGE.h>
#include <TorqueLib/QuickOverride.h>
#include "mapper.h"

static Mapper *portMapper = nullptr;

PLUGINCALLBACK void preEngineInit(PluginInterface *plugin)
{
}

PLUGINCALLBACK void postEngineInit(PluginInterface *plugin)
{
	portMapper = new Mapper();
}

PLUGINCALLBACK void engineShutdown(PluginInterface *plugin)
{
	portMapper->stop();
}

TorqueOverride(TGE::Net::Error, Net::bind, (NetSocket sock, U16 port), originalBind) {
	TGE::Net::Error status = originalBind(sock, port);

	if (status == TGE::Net::NoError) {
		DEBUG_PRINTF("Opening port %d", port);

		//Try to open a port for them
		portMapper->tryMap(port, port, Mapper::PortProtocol::PortProtocolUDP, false);
	}
	return status;
	//Port map!
}
