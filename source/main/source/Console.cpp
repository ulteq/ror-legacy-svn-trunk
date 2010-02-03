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
#include "Console.h"
#include "ScriptEngine.h"
#include "InputEngine.h"

Console::Console()
{
	initialiseByAttributes(this);

	// make it 1/3 of screen height and place over screen
	MyGUI::IntSize size = MyGUI::Gui::getInstance().getViewSize();
	size.height = size.height/3;
	mMainWidget->setCoord(0, -size.height, size.width, size.height);

	mCommandEdit->eventKeyButtonPressed = MyGUI::newDelegate(this, &Console::eventButtonPressed);
	mCommandEdit->eventEditSelectAccept = MyGUI::newDelegate(this, &Console::eventCommandAccept);

	mHistoryPosition = 0;
	history.push_back("");

	setVisible(false);
}

Console::~Console()
{
}

void Console::setVisible(bool _visible)
{
	mVisible = _visible;

	mMainWidget->setEnabledSilent(_visible);
	if (_visible)
	{
		MyGUI::InputManager::getInstance().setKeyFocusWidget(mCommandEdit);
	}
	else
	{
		MyGUI::InputManager::getInstance().resetKeyFocusWidget(mCommandEdit);
	}
}

bool Console::getVisible()
{
	return mVisible;
}

void Console::print(const MyGUI::UString &_text)
{
	if (!mLogEdit->getCaption().empty())
		mLogEdit->addText("\n");
	mLogEdit->addText(_text);

	mLogEdit->setTextSelection(mLogEdit->getTextLength(), mLogEdit->getTextLength());
}

void Console::eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
	if(_key == MyGUI::KeyCode::Escape || _key == MyGUI::KeyCode::Enum(INPUTENGINE.getKeboardKeyForCommand(EV_COMMON_CONSOLEDISPLAY)))
	{
		setVisible(false);
		// delete last character (to avoid printing `)
		size_t lastChar = mCommandEdit->getTextLength() - 1;
		if (mCommandEdit->getCaption()[lastChar] == '`')
			mCommandEdit->eraseText(lastChar);
		return;
	}

	switch(_key.toValue())
	{
	case MyGUI::KeyCode::ArrowUp:
		if(mHistoryPosition > 0)
		{
			// first we save what we was writing
			if (mHistoryPosition == (int)history.size() - 1)
			{
				history[mHistoryPosition] = mCommandEdit->getCaption();
			}
			mHistoryPosition--;
			mCommandEdit->setCaption(history[mHistoryPosition]);
		}
		break;

	case MyGUI::KeyCode::ArrowDown:
		if(mHistoryPosition < (int)history.size() - 1)
		{
			mHistoryPosition++;
			mCommandEdit->setCaption(history[mHistoryPosition]);
		}
		break;

	case MyGUI::KeyCode::PageUp:
		if (mLogEdit->getVScrollPosition() > (size_t)mLogEdit->getHeight())
			mLogEdit->setVScrollPosition(mLogEdit->getVScrollPosition() - mLogEdit->getHeight());
		else
			mLogEdit->setVScrollPosition(0);
		break;

	case MyGUI::KeyCode::PageDown:
		mLogEdit->setVScrollPosition(mLogEdit->getVScrollPosition() + mLogEdit->getHeight());
		break;
	}
}

void Console::eventCommandAccept(MyGUI::Edit* _sender)
{
	MyGUI::UString command = _sender->getCaption();

	// special command
	if(command == "hide")
	{
		setVisible(false);
	}
	else
	{
		print(command);
#ifdef ANGELSCRIPT
		ScriptEngine::getSingleton().executeString(command);
#endif //ANGELSCRIPT
	}
	*history.rbegin() = command;
	history.push_back(""); // new, empty last entry
	mHistoryPosition = history.size() - 1; // switch to the new line
}