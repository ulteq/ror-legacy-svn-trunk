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
#include "ForceFeedback.h"

ForceFeedback::ForceFeedback(OIS::ForceFeedback* ffdevice, Real overall_gain, Real stress_gain, Real centering_gain, Real camera_gain)
{
	this->ffdevice=ffdevice;
	this->overall_gain=overall_gain;
	this->stress_gain=stress_gain;
	this->centering_gain=centering_gain;
	this->camera_gain=camera_gain;
	LogManager::getSingleton().logMessage(String("ForceFeedback: ")+StringConverter::toString(ffdevice->getFFAxesNumber())+" axe(s)");
	const OIS::ForceFeedback::SupportedEffectList &supEffects=ffdevice->getSupportedEffects();
	if (supEffects.size()>0)
	{
		LogManager::getSingleton().logMessage("ForceFeedback: supported effects:");
		OIS::ForceFeedback::SupportedEffectList::const_iterator efit;
		for(efit=supEffects.begin(); efit!=supEffects.end(); ++efit)
			LogManager::getSingleton().logMessage(String("ForceFeedback: ")+OIS::Effect::getEffectTypeName(efit->second));
	}
	else
		LogManager::getSingleton().logMessage("ForceFeedback: no supported effect found!");
	ffdevice->setAutoCenterMode(false);
	ffdevice->setMasterGain(0.0);
	enabled_state=false;
	//do not load effect now, its too early
	hydroEffect=0;

}


void ForceFeedback::setForces(Real roll, Real pitch, Real wspeed, Real dircommand, Real stress)
{
	//LogManager::getSingleton().logMessage(String("ForceFeedback: R=")+StringConverter::toString(roll)+" D="+StringConverter::toString(dir)+" S="+StringConverter::toString(wspeed)+" H="+StringConverter::toString(stress));
	if (!hydroEffect)
	{
		//we create effect at the last moment, because it does not works otherwise
		hydroEffect=new OIS::Effect(OIS::Effect::ConstantForce, OIS::Effect::Constant);
		hydroEffect->direction = OIS::Effect::North;
		hydroEffect->trigger_button = 0;
		hydroEffect->trigger_interval = 0;
		hydroEffect->replay_length = OIS::Effect::OIS_INFINITE; // Linux/Win32: Same behaviour as 0.
		hydroEffect->replay_delay = 0;
		hydroEffect->setNumAxes(1);
		OIS::ConstantEffect* hydroConstForce = dynamic_cast<OIS::ConstantEffect*>(hydroEffect->getForceEffect());
		hydroConstForce->level = 0;  //-10K to +10k
		hydroConstForce->envelope.attackLength = 0;
		hydroConstForce->envelope.attackLevel = (unsigned short)hydroConstForce->level;
		hydroConstForce->envelope.fadeLength = 0;
		hydroConstForce->envelope.fadeLevel = (unsigned short)hydroConstForce->level;

		ffdevice->upload(hydroEffect);
	}
	
	OIS::ConstantEffect* hydroConstForce = dynamic_cast<OIS::ConstantEffect*>(hydroEffect->getForceEffect());
	Real ff=-stress*stress_gain*4.0+dircommand*100.0*centering_gain*wspeed*wspeed;
	if (ff>10000) ff=10000;
	if (ff<-10000) ff=-10000;
	hydroConstForce->level = ff;  //-10K to +10k
	ffdevice->modify(hydroEffect);
	
}

void ForceFeedback::setEnabled(bool b)
{
	if (b!=enabled_state)
	{
		if (b) 	ffdevice->setMasterGain(overall_gain);
		else ffdevice->setMasterGain(0.0);
		enabled_state=b;
	}
}

ForceFeedback::~ForceFeedback()
{
	setEnabled(false);
}

