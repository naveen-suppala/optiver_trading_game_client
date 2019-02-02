#include "execution_client.h"
#include "tsc_clock.h"

#include <cstring>
#include <cerrno>
#include <stdexcept>

extern "C"
{
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
}

#include <iostream>

using namespace std::string_literals;

extern TSCTimestamp TimestampIn;

///////////////////////
// NOTE: do NOT edit this line
static const std::string Username = "USERNAME";
///////////////////////

ExecutionClient::ExecutionClient(uint16_t localPort, std::string remoteHost, uint16_t remotePort)
{
	if ((mFD = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		throw std::runtime_error("execution client: socket() failed: "s + std::strerror(errno));
	}

	Address sourceAddr;
	std::memset(&sourceAddr, 0, sizeof(sourceAddr));
	sourceAddr.sin_family = AF_INET;
	sourceAddr.sin_port = htons(localPort);
	sourceAddr.sin_addr.s_addr =  htonl(INADDR_ANY);

	if (::bind(mFD, (sockaddr*)&sourceAddr, sizeof(sourceAddr)) < 0)
	{
		throw std::runtime_error("execution client: bind() failed: "s + std::strerror(errno));
	}

	std::memset(&mRemote, 0, sizeof(mRemote));
	mRemote.sin_family = AF_INET;
	mRemote.sin_port = htons(remotePort);
	mRemote.sin_addr.s_addr = ::inet_addr(remoteHost.data());

	if (mRemote.sin_addr.s_addr == INADDR_NONE)
	{
		throw std::runtime_error("execution client: inet_addr() failed: "s + std::strerror(errno));
	}
}

void ExecutionClient::Send(OrderMessage order)
{
	std::cout << "sending order, " << order.mAction << " "
			  << order.mVolume.mValue << "@" << order.mPrice.mValue << " "
			  << order.mFeedcode << std::endl;

	std::string message;
	message = "TYPE=ORDER|USERNAME=";
	message += Username;
	message += "|FEEDCODE=";
	message += order.mFeedcode;
	message += "|ACTION=";
	message += order.mAction;
	message += "|PRICE=";
	message += std::to_string(order.mPrice.mValue);
	message += "|VOLUME=";
	message += std::to_string(order.mVolume.mValue);

	SendSerializedMessage(message);
}

void ExecutionClient::SendSerializedMessage(std::string data)
{
	///////////////////////
	// NOTE: this block can be modified, as long as the timestamp measurement stay
	// the LAST operation before calling sendto()
	{
		const TSCTimestamp timestampOut{TSCClock::Now()};
		data += "|LATENCY=" + std::to_string(TSCClock::FromCycles(timestampOut.mValue - TimestampIn.mValue).count());
	}
	//////////////////////

	if (::sendto(mFD, data.data(), data.size(), 0, (sockaddr*)&mRemote, sizeof(mRemote)) < 0)
	{
		throw std::runtime_error("execution client: sendto() failed:"s + std::strerror(errno));
	}
}
