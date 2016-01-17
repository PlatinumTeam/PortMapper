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

#include "mapper.h"
#include <sys/types.h>
#include <map>

#ifdef _WIN32
#include <WinSock2.h>
#include <winsock.h>
#endif

bool Mapper::canMapNatPMP = false;
bool Mapper::canMapUPnP = false;

Mapper::Mapper() {
	detectProtocol();
}

void Mapper::stop() {
}

void Mapper::detectProtocol() {
	canMapNatPMP = false;
	canMapUPnP = false;
//	std::thread(&Mapper::_detectNatPMP).detach();
//	std::thread(&Mapper::_detectUPnP).detach();
}

void Mapper::_detectUPnP() {
	return;
	
	UPNPDev *devicelist = NULL;
	const char *multicastif = NULL;
	const char *minissdpdpath = NULL;

	char lanaddr[16];
	char external[16];

	UPNPUrls urls;
	IGDdatas datas;

	//Check device
	if ((devicelist = upnpDiscover(2000, multicastif, minissdpdpath, 0))) {
		if (devicelist) {
			bool foundIGD = false;
			UPNPDev *device;

			std::vector<const char *>urlsToTry;
			std::vector<const char *>triedURLs;

			for (device = devicelist; device && !foundIGD; device = device->pNext) {
				const char *descURL = device->descURL;
				urlsToTry.push_back(descURL);
			}
			for (const char *url : urlsToTry) {
				if (urls.controlURL)
					FreeUPNPUrls(&urls);

				if (UPNP_GetIGDFromUrl(url, &urls, &datas, lanaddr, sizeof(lanaddr))) {
					int r = UPNP_GetExternalIPAddress(urls.controlURL, datas.servicetype, external);

					if (r != UPNPCOMMAND_SUCCESS)  {
						canMapUPnP = false;
					} else {
						if (external[0]) {
							canMapUPnP = true;
							break;
						} else {
							canMapNatPMP = false;
						}
					}
				}
			}
		}
	}
}

void Mapper::_detectNatPMP() {
	
}

bool Mapper::tryMap(U16 localPort, U16 externalPort, PortProtocol protocol, bool remove) {
	if (!this->canMapUPnP || !this->canMapNatPMP)
		return false;

	MapData *data = new MapData;
	data->localPort = localPort;
	data->externalPort = externalPort;
	data->protocol = protocol;
	data->remove = remove;

	//Post it on the thread
	if (this->canMapNatPMP) {
//		std::thread(&Mapper::_tryMapNatPMP, std::ref(data)).detach();
	} else if (this->canMapUPnP) {
//		std::thread(&Mapper::_tryMapUPnP, data).detach();
	}

	return true;
}

void *Mapper::_tryMapNatPMP(Mapper::MapData *data) {
	U16 localPort = data->localPort;
	U16 externalPort = data->externalPort;
	Mapper::PortProtocol protocol = data->protocol;
	bool remove = data->remove;

	if (protocol == PortProtocolBoth) {
		data->protocol = PortProtocolTCP;
		_tryMapNatPMP(data);
		data->protocol = PortProtocolUDP;
		_tryMapNatPMP(data);

		delete data;
		return NULL;
	}

	natpmp_t natPMP;
	initnatpmp(&natPMP);

	int ret = sendnewportmappingrequest(&natPMP, protocol == PortProtocolTCP ? NATPMP_PROTOCOL_TCP : NATPMP_PROTOCOL_UDP, localPort, externalPort, remove ? 0 : 3600);
	fd_set fds;
	timeval timeout;
	natpmpresp_t response;

	do {
		FD_ZERO(&fds);
		FD_SET(natPMP.s, &fds);
		getnatpmprequesttimeout(&natPMP, &timeout);
		select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
		ret = readnatpmpresponseorretry(&natPMP, &response);
	} while (ret == NATPMP_TRYAGAIN);

	if (ret < 0) {
		DEBUG_ERRORF("[Nat-PMP] Port map failed: Error %d", ret);
	} else {
		DEBUG_PRINTF("[Nat-PMP] Port map succeded, mapped port is %d to %d", localPort, externalPort);
	}

	closenatpmp(&natPMP);

	delete data;
	return NULL;
}

void *Mapper::_tryMapUPnP(Mapper::MapData *data) {
	U16 localPort = data->localPort;
	U16 externalPort = data->externalPort;
	Mapper::PortProtocol protocol = data->protocol;
	bool remove = data->remove;

	if (protocol == PortProtocolBoth) {
		data->protocol = PortProtocolTCP;
		_tryMapUPnP(data);
		data->protocol = PortProtocolUDP;
		_tryMapUPnP(data);

		delete data;
		return NULL;
	}

	UPNPDev *devicelist = NULL;
	const char *multicastif = NULL;
	const char *minissdpdpath = NULL;

	const char *descURL = NULL;
	char lanaddr[16];

	//Check device
	if ((devicelist = upnpDiscover(2000, multicastif, minissdpdpath, 0))) {
		if (devicelist) {
			bool foundIGD = false;
			UPNPDev *device;

			for (device = devicelist; device && !foundIGD; device = device->pNext) {
				descURL = device->descURL;
			}
		}
	}

	UPNPUrls urls;
	IGDdatas datas;

	if (UPNP_GetIGDFromUrl(descURL, &urls, &datas, lanaddr, sizeof(lanaddr))) {
		if (remove) {
			char port[6];
			sprintf(port, "%d", externalPort);
			UPNP_DeletePortMapping(urls.controlURL, datas.servicetype, port, (protocol == PortProtocolTCP ? "TCP" : "UDP"), NULL);

			delete data;
			return NULL;
		}

		int ret;
		int mappedPort = externalPort;
		do {
			char external[6];
			char internal[6];
			sprintf(external, "%d", mappedPort);
			sprintf(internal, "%d", localPort);

			ret = UPNP_AddPortMapping(urls.controlURL, datas.servicetype, external, internal, lanaddr, "", (protocol == PortProtocolTCP ? "TCP" : "UDP"), NULL);
			
			if (ret != UPNPCOMMAND_SUCCESS) {
				switch (ret) {
					case 718:
						DEBUG_ERRORF("[UPnP] Port map failed: Port conflict. Trying next port.");
						mappedPort ++;
						break;
					case 724:
						DEBUG_ERRORF("[UPnP] Port map failed: SamePortValuesRequired");
						break;
					case 725:
						DEBUG_ERRORF("[UPnP] Port map failed: OnlyPermanentLeasesSupported");
						break;
					case 727:
						DEBUG_ERRORF("[UPnP] Port map failed: ExternalPortOnlySupportsWildcard");
						break;
					default:
						break;
				}
			}
		} while (ret != UPNPCOMMAND_SUCCESS && ret == 718 && mappedPort - externalPort <= 40);

		if (ret == UPNPCOMMAND_SUCCESS) {
			DEBUG_PRINTF("[UPnP] Port map succeded, mapped port %d to %d", localPort, mappedPort);
		}
	}

	delete data;
	return NULL;
}
