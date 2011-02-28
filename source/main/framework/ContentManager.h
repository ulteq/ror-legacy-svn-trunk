/*
This source file is part of Rigs of Rods
Copyright 2005-2011 Pierre-Michel Ricordel
Copyright 2007-2011 Thomas Fischer

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

#ifndef CONTENTMANAGER_H__
#define CONTENTMANAGER_H__

#include <OgreSingleton.h>

#include <OgreResourceGroupManager.h>
//#include "RoR

class ContentManager : public Ogre::Singleton<ContentManager>, public Ogre::ResourceLoadingListener
{
public:
	ContentManager();
	~ContentManager();

	bool init(void);

protected:
	ContentManager(const ContentManager&);
	ContentManager& operator= (const ContentManager&);

	void loadMainResource(Ogre::String name, Ogre::String group=Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	void exploreFolders(Ogre::String rg);

	// implementation for resource loading listener
	Ogre::DataStreamPtr resourceLoading(const Ogre::String &name, const Ogre::String &group, Ogre::Resource *resource);
	void resourceStreamOpened(const Ogre::String &name, const Ogre::String &group, Ogre::Resource *resource, Ogre::DataStreamPtr& dataStream);
	bool resourceCollision(Ogre::Resource *resource, Ogre::ResourceManager *resourceManager);

};

#endif // CONTENTMANAGER_H__
