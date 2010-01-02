/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of August 2009

#ifndef STREAMABLE_H__
#define STREAMABLE_H__

#include "OgrePrerequisites.h"
#include "rornet.h"
#include <map>
#include "pthread.h"


class Network;
class NetworkStreamManager;

typedef struct recvPacket_t
{
	header_t header;
	char *buffer[MAX_MESSAGE_LENGTH];
} recvPacket_t;

/**
 * This class defines a standart interface and a buffer between the actual network code and the class that handles it.
 * The buffer must be decoupled from the separatly running network thread.
 */
class Streamable
{
	friend class NetworkStreamManager;
public:
	unsigned int getStreamID() { return this->streamid; };
	unsigned int getSourceID() { return this->sourceid; };
	void setStreamID(unsigned int id) { this->streamid=id; };
	void setSourceID(unsigned int id) { this->sourceid=id; };

protected:
	// constructor/destructor are protected, so you cannot create instances without using the factory
	Streamable();
	~Streamable();

	// static const members
	static const unsigned int packetBufferSize = 30; //!< hard buffer size limit
	static const unsigned int packetBufferSizeDiscardData = 20; //!< we will discard all data packets after hitting this border. Then we have still some packets left for registration purposes.
	static const unsigned int maxPacketLen     = 8192;

	// custom types
	typedef struct _bufferedPacket
	{
		char packetBuffer[maxPacketLen];
		unsigned int size;
	} bufferedPacket_t;

	// normal members
	std::deque < bufferedPacket_t > packets;
	std::deque < recvPacket_t > receivedPackets;
	
	unsigned int sourceid, streamid;


	// virtual interface methods
	virtual void sendStreamData() = 0;
	virtual void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len) = 0;

	// base class methods
	void addPacket(int type, unsigned int len, char *content);
	void addReceivedPacket(header_t header, char *buffer);

	std::deque < bufferedPacket_t > *getPacketQueue();
	std::deque < recvPacket_t > *getReceivePacketQueue();
	pthread_mutex_t recv_work_mutex;

	void lockReceiveQueue();
	void unlockReceiveQueue();
};

#endif //STREAMABLE_H__
