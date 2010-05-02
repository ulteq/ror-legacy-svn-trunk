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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 1st of May 2010

#ifndef MESHOBJECT_H__
#define MESHOBJECT_H__

#include "Ogre.h"
#include "rormemory.h"

class MeshObject : public Ogre::ResourceBackgroundQueue::Listener, public Ogre::Resource::Listener
{
public:
	MeshObject(Ogre::SceneManager *smgr, Ogre::String meshName, Ogre::String entityName, Ogre::SceneNode *sceneNode=0, bool backgroundLoading=true) : \
		smgr(smgr), entityName(entityName), meshName(meshName), sceneNode(sceneNode), ent(0), backgroundLoading(backgroundLoading), loaded(false), 
		enableSimpleMaterial(false), materialName(), skin(0), castshadows(true), mfm(0)
	{
		// create a new sceneNode if not existing
		if(!sceneNode)
			sceneNode = smgr->getRootSceneNode()->createChildSceneNode();

		loadMesh();
	}

	~MeshObject()
	{
       if(backgroundLoading && !mesh.isNull())
           mesh->unload();
	}

	void setSimpleMaterialColour(Ogre::ColourValue c)
	{
		simpleMatColour = c;
		enableSimpleMaterial = true;
		if(loaded && ent)
		{
			// already loaded, so do it afterwards manually
			MaterialFunctionMapper::replaceSimpleMeshMaterials(ent, simpleMatColour);
		}
	}

	void setSkin(Skin *s)
	{
		if(!s) return;
		skin = s;
		if(loaded && ent)
		{
			// already loaded, so do it afterwards manually
			skin->replaceMeshMaterials(ent);
		}

	}

	void setMaterialFunctionMapper(MaterialFunctionMapper *m)
	{
		if(!m) return;
		mfm = m;
		if(loaded && ent)
		{
			// already loaded, so do it afterwards manually
			mfm->replaceMeshMaterials(ent);
		}
	}

	void setMaterialName(Ogre::String m)
	{
		if(m.empty()) return;
		materialName = m;
		if(loaded && ent)
		{
			ent->setMaterialName(materialName);
		}
	}

	void setCastShadows(bool b)
	{
		castshadows=b;
		if(loaded && sceneNode && ent && sceneNode->numAttachedObjects())
		{
			sceneNode->getAttachedObject(0)->setCastShadows(b);
		}

	}
protected:
	Ogre::SceneManager *smgr;
	Ogre::String meshName, entityName;
	Ogre::SceneNode *sceneNode;
	Ogre::Entity *ent;
	Ogre::MeshPtr mesh;
	Ogre::BackgroundProcessTicket ticket;
	bool backgroundLoading;
	bool loaded;
	
	Ogre::ColourValue simpleMatColour;
	bool enableSimpleMaterial;
	Ogre::String materialName;
	Skin *skin;
	bool castshadows;
	MaterialFunctionMapper *mfm;

	void postProcess()
	{
		loaded=true;

		if(enableSimpleMaterial)
			MaterialFunctionMapper::replaceSimpleMeshMaterials(ent, simpleMatColour);

		if(skin)
			skin->replaceMeshMaterials(ent);

		if(mfm)
			mfm->replaceMeshMaterials(ent);

		if(!materialName.empty())
			ent->setMaterialName(materialName);

		// only set it if different from default (true)
		if(!castshadows)
			sceneNode->getAttachedObject(0)->setCastShadows(castshadows);


		sceneNode->setVisible(true);
	}

	void loadMesh()
	{
		Ogre::String resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
		Ogre::MeshPtr mesh = static_cast<Ogre::MeshPtr>(Ogre::MeshManager::getSingletonPtr()->create(meshName, resourceGroup));
		if(backgroundLoading)
		{
			mesh->setBackgroundLoaded(true);
			mesh->addListener(this);
			ticket = Ogre::ResourceBackgroundQueue::getSingleton().load(
				Ogre::MeshManager::getSingletonPtr()->getResourceType(), 
				mesh->getName(), 
				resourceGroup,
				false,
				0,
				0,
				0);
		}

		// now create an entity around the mesh and attach it to the scene graph
		try
		{
			if(entityName.empty())
				ent = smgr->createEntity(meshName);
			else
				ent = smgr->createEntity(entityName, meshName);
			if(ent)
				sceneNode->attachObject(ent);
		} catch(Ogre::Exception& e)
		{
			LogManager::getSingleton().logMessage("error loading mesh: " + meshName + ": " + e.getFullDescription());
			return;
		}
		
		if(!backgroundLoading)
			postProcess();
	}

	void operationCompleted(BackgroundProcessTicket ticket, const BackgroundProcessResult& result)
	{
		LogManager::getSingleton().logMessage("operationCompleted: " + meshName);
		if(ticket == this->ticket)
			postProcess();
	}

	void backgroundLoadingComplete(Resource *r)
	{
		// deprecated, use loadingComplete instead
	}

	void backgroundPreparingComplete(Resource *r)
	{
		// deprecated, use preparingComplete instead
	}

	void loadingComplete(Resource *r)
	{
		LogManager::getSingleton().logMessage("loadingComplete: " + meshName);
		postProcess();
	}

	void preparingComplete(Resource *r)
	{
		LogManager::getSingleton().logMessage("preparingComplete: " + meshName);
	}

	void unloadingComplete(Resource *r)
	{
		LogManager::getSingleton().logMessage("unloadingComplete: " + meshName);
	}
};

#endif //MESHOBJECT_H__
