/*/*************************************************************************
 *                                                                         *
 * UniversumLib, collection of classes for generating and go through a     *
 * whole universe. It is for my Gameproject Spacecraft					   *
 * Copyright (C) 2014, 2015, 2016, 2017 Dario Rekowski.					   *
 * Email: dario.rekowski@gmx.de   Web: www.spacecrafting.de                *
 *                                                                         *
 * This program is free software: you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * any later version.													   *
 *																		   *
 * This program is distributed in the hope that it will be useful,	       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of	       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	       *
 * GNU General Public License for more details.							   *
 *																		   *
 * You should have received a copy of the GNU General Public License	   *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *                                                                         *
 ***************************************************************************/
 //#include "UniversumLib.h"

#ifndef __UNIVERSUM_LIB_LIB_TIMER__
#define __UNIVERSUM_LIB_LIB_TIMER__

#include "Poco/Mutex.h"
#include "Poco/Thread.h"
#include <string>
#include <map>

// refactored from my game engine for gradido
// @date: 04.12.21
// @author: einhornimmond

// MAGIC NUMBER: The timer has a list of all timer callback, sorted which should be called when
// on every move call it will check if the current callback time is reached and if so, it will be called
// the time between move calls depends on needed accuracy and timer calls per seconds
#define MAGIC_NUMBER_TIMER_THREAD_SLEEP_BETWEEN_MOVE_CALLS_MILLISECONDS 100


enum TimerReturn {
	GO_ON = 0,
	REMOVE_ME = 1,
	REPORT_ERROR = 2
};
#if (_MSC_VER >= 1200 && _MSC_VER < 1310)
enum TimerReturn;
#endif

class TimerCallback
{
public:
	virtual TimerReturn callFromTimer() = 0;
	virtual const char* getResourceType() const { return "TimerCallback"; };
};

class FuzzyTimer : public Poco::Runnable
{
public:
	FuzzyTimer();
	~FuzzyTimer();

	/*!
		add timer callback object
		\param timeIntervall intervall in milliseconds
		\param loopCount 0 for one call, -1 for loop
	*/
	bool addTimer(std::string name, TimerCallback* callbackObject, uint64_t timeIntervall, int loopCount = -1);

	/*!
		\brief remove all timer with name
		function is not really fast, because all appended timerCallback will be checked

		\return removed timer, or -1 by error
	*/
	int removeTimer(std::string name);

	/*
		\brief update timer map, maybe call timer... (only one per frame)
	*/
	bool move();

	void run();
private:
	struct TimerEntry {
		// functions
		TimerEntry(TimerCallback* _callback, uint64_t _timeIntervall, int _loopCount, std::string _name)
			: callback(_callback), timeIntervall(_timeIntervall), initLoopCount(_loopCount), currentLoopCount(0), name(_name) {}
		~TimerEntry() {}
		// \return true if we can run once again
		bool nextLoop() {
			currentLoopCount++;
			if (initLoopCount < 0 || initLoopCount - 1 > currentLoopCount) return true;
			return false;
		}

		// variables
		TimerCallback* callback;
		uint64_t timeIntervall;
		int initLoopCount;
		int currentLoopCount;
		std::string name;
	};
	// int key = time since programm start to run
	std::multimap<uint64_t, TimerEntry> mRegisteredAtTimer;
	typedef std::pair<uint64_t, TimerEntry> TIMER_TIMER_ENTRY;
	Poco::Mutex mMutex;
	bool		exit;
	Poco::Thread mThread;
};


#endif //__UNIVERSUM_LIB_LIB_TIMER__