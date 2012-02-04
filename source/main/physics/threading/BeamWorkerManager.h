/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
#ifndef __BeamWorkerManager_H__
#define __BeamWorkerManager_H__

#include "RoRPrerequisites.h"
#include "OgrePrerequisites.h"

#include "BeamFactory.h"
#include "Singleton.h"

class BeamThread;

// Manager that manages all threads and the beam locking
class BeamWorkerManager : public RoRSingleton<BeamWorkerManager>
{
	friend class BeamThread;
	friend class RoRSingleton<BeamWorkerManager>;
protected:
	std::vector<BeamThread*> threads;
	int threadsSize;
	int done_count;
	pthread_mutex_t work_mutex;
	pthread_cond_t work_cv;
	pthread_mutex_t done_count_mutex;
	pthread_cond_t done_count_cv;
	pthread_t workerThread;

	BeamWorkerManager();
	~BeamWorkerManager();

	void addThread(BeamThread *bthread);
	void removeThread(BeamThread *bthread);
	void syncThreads(BeamThread *bthread);

public:
	void _startWorkerLoop();
};

#endif //__BeamWorkerManager_H__
