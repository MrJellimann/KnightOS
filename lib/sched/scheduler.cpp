//
// scheduler.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <circle/sched/scheduler.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromScheduler[] = "sched";

CScheduler *CScheduler::s_pThis = 0;

CScheduler::CScheduler (void)
:	m_nTasks (0),
	m_pCurrent (0),
	m_nCurrent (0),
	m_pTaskSwitchHandler (0),
	m_pTaskTerminationHandler (0)
{
	assert (s_pThis == 0);
	s_pThis = this;

	m_pCurrent = new CTask (0);		// main task currently running
	assert (m_pCurrent != 0);
	print = false;

	// TODO: make comparison function for PQ
	// TODO: instantiate all the queues
	
}

CScheduler::~CScheduler (void)
{
	m_pTaskSwitchHandler = 0;
	m_pTaskTerminationHandler = 0;

	s_pThis = 0;


}

void CScheduler::Yield (void)
{
	while ((m_nCurrent = GetNextTask ()) == MAX_TASKS)	// no task is ready
	{
		assert (m_nTasks > 0);
	}

	assert (m_nCurrent < MAX_TASKS);
	CTask *pNext = m_pTask[m_nCurrent];
	assert (pNext != 0);
	if (m_pCurrent == pNext)
	{
		return;
	}
	
	TTaskRegisters *pOldRegs = m_pCurrent->GetRegs ();
	m_pCurrent = pNext;
	TTaskRegisters *pNewRegs = m_pCurrent->GetRegs ();

	if (m_pTaskSwitchHandler != 0)
	{
		(*m_pTaskSwitchHandler) (m_pCurrent);
	}

	assert (pOldRegs != 0);
	assert (pNewRegs != 0);
	TaskSwitch (pOldRegs, pNewRegs);
}

void CScheduler::Sleep (unsigned nSeconds)
{
	// be sure the clock does not run over taken as signed int
	const unsigned nSleepMax = 1800;	// normally 2147 but to be sure
	while (nSeconds > nSleepMax)
	{
		usSleep (nSleepMax * 1000000);

		nSeconds -= nSleepMax;
	}

	usSleep (nSeconds * 1000000);
}

void CScheduler::MsSleep (unsigned nMilliSeconds)
{
	if (nMilliSeconds > 0)
	{
		usSleep (nMilliSeconds * 1000);
	}
}

void CScheduler::usSleep (unsigned nMicroSeconds)
{
	if (nMicroSeconds > 0)
	{
		unsigned nTicks = nMicroSeconds * (CLOCKHZ / 1000000);

		unsigned nStartTicks = CTimer::Get ()->GetClockTicks ();

		assert (m_pCurrent != 0);
		assert (m_pCurrent->GetState () == TaskStateReady);
		m_pCurrent->SetWakeTicks (nStartTicks + nTicks);
		m_pCurrent->SetState (TaskStateSleeping);

		Yield ();
	}
}

CTask *CScheduler::GetCurrentTask (void)
{
	return m_pCurrent;
}

void CScheduler::RegisterTaskSwitchHandler (TSchedulerTaskHandler *pHandler)
{
	assert (m_pTaskSwitchHandler == 0);
	m_pTaskSwitchHandler = pHandler;
	assert (m_pTaskSwitchHandler != 0);
}

void CScheduler::RegisterTaskTerminationHandler (TSchedulerTaskHandler *pHandler)
{
	assert (m_pTaskTerminationHandler == 0);
	m_pTaskTerminationHandler = pHandler;
	assert (m_pTaskTerminationHandler != 0);
}



void CScheduler::AddTask (CTask *pTask)
{
	assert (pTask != 0);
	// add according to weight if weight isnt zero
	unsigned i;
	for (i = 0; i < m_nTasks; i++)
	{
		
		if(pTask->GetWeight() == 0){
			if (m_pTask[i] == 0)
			{
				m_pTask[i] = pTask;

				return;
			}
			continue;
		}
		if(pTask->GetWeight() > m_pTask[i]->GetWeight()){
			// swap tasks to go down queue
			CTask *tempTask = m_pTask[i];
			m_pTask[i] = pTask;
			pTask = tempTask;
		}
	}

	if (m_nTasks >= MAX_TASKS)
	{
		CLogger::Get ()->Write (FromScheduler, LogPanic, "System limit of tasks exceeded");
	}

	m_pTask[m_nTasks++] = pTask;
}

void CScheduler::RemoveTask (CTask *pTask)
{
	for (unsigned i = 0; i < m_nTasks; i++)
	{
		if (m_pTask[i] == pTask)
		{
			m_pTask[i] = 0;

			if (i == m_nTasks-1)
			{
				m_nTasks--;
			}

			return;
		}
	}

	assert (0);
}

void CScheduler::PopTask()
{
	// DO NOT UNCOMMENT THIS UNLESS YOU KNOW WHAT YOU'RE DOING
	// POPPING THE TASK AT [0] KILLS THE SYSTEM, FORCING MANUAL RESTART

	// m_pTask[0] = 0;
	
	// if (m_nTasks > 0)
	// 	m_nTasks--;
	
	// if (0 == m_nTasks-1)
	// {
	// 	m_nTasks--;
	// }
	return;
}

void CScheduler::BlockTask (CTask **ppTask)
{
	assert (ppTask != 0);
	*ppTask = m_pCurrent;

	assert (m_pCurrent != 0);
	assert (m_pCurrent->GetState () == TaskStateReady);
	m_pCurrent->SetState (TaskStateBlocked);

	Yield ();
}

void CScheduler::WakeTask (CTask **ppTask)
{
	assert (ppTask != 0);
	CTask *pTask = *ppTask;

	*ppTask = 0;

#ifdef NDEBUG
	if (   pTask == 0
	    || pTask->GetState () != TaskStateBlocked)
	{
		CLogger::Get ()->Write (FromScheduler, LogPanic, "Tried to wake non-blocked task");
	}
#else
	assert (pTask != 0);
	assert (pTask->GetState () == TaskStateBlocked);
#endif

	pTask->SetState (TaskStateReady);
}

//TODO: some sort of trading off between userQ and sysQ?
// checking for aging, reweighting, rearraging?
// if no sys tasks for directly to userQ, else do sys task
unsigned CScheduler::GetNextTask (void)
{
	unsigned nTask = m_nCurrent < MAX_TASKS ? m_nCurrent : 0;

	unsigned nTicks = CTimer::Get ()->GetClockTicks ();

	for (unsigned i = 1; i <= m_nTasks; i++)
	{
		if (++nTask >= m_nTasks)
		{
			nTask = 0;
		}

		CTask *pTask = m_pTask[nTask];
		if (pTask == 0)
		{
			continue;
		}

		switch (pTask->GetState ())
		{
		case TaskStateReady:
			return nTask;

		case TaskStateBlocked:
			continue;

		case TaskStateSleeping:
			if ((int) (pTask->GetWakeTicks () - nTicks) > 0)
			{
				continue;
			}
			pTask->SetState (TaskStateReady);
			return nTask;

		case TaskStateTerminated:
			if (m_pTaskTerminationHandler != 0)
			{
				(*m_pTaskTerminationHandler) (pTask);
			}
			RemoveTask (pTask);
			delete pTask;
			return MAX_TASKS;

		default:
			assert (0);
			break;
		}
	}

	return MAX_TASKS;
}

CScheduler *CScheduler::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}


void CScheduler::turnPrintOff()
{
	print = false;
	return;
}

void CScheduler::turnPrintOn()
{
	print = true;
	return;
}

boolean CScheduler::getPrint()
{
	return print;
}

// This one prints directly to the screen
CString CScheduler::listTasks()
{
	CString Message, _temp;
	
	_temp.Format("Current Task Index: %i || Task Count: %i || Total Possible Tasks: %i\n\n", m_nCurrent, m_nTasks, MAX_TASKS);
	Message.Append((const char*) _temp);

	unsigned int i;
	int tempWeight;
	// Using m_nTasks here prevents a stack overflow and kernel panic so be nice!
	for (i = 0; i < m_nTasks; i++)
	{
		tempWeight = m_pTask[i]->GetWeight();

		if (tempWeight < 0)
			tempWeight = -1;
		
		// Format the string
		// _temp.Format ("Index: %d | TaskWeight: %d\n", i, tempWeight);
		_temp.Format ("Index: %d | Weight: %d | Addr: %x\n", i, tempWeight, m_pTask[i]);

		// Add to message
		Message.Append((const char*) _temp);
	}

	// Send off message
	return Message;
}

// This one uses the logger to get it done
void CScheduler::ListTasks()
{
	unsigned int i;
	int tempWeight;

	CLogger::Get()->Write(FromScheduler, LogNotice, "Current Task Index: %i || Task Count: %i || Total Possible Tasks: %i\n", m_nCurrent, m_nTasks, MAX_TASKS);

	// Using m_nTasks here prevents a stack overflow and kernel panic so be nice!
	for (i = 0; i < m_nTasks; i++)
	{
		if (m_pTask[i] == 0)
		{
			tempWeight = -1;
		}
		else
		{
			tempWeight = m_pTask[i]->GetWeight();

			if (tempWeight < 0)
				tempWeight = -1;
		}
		
		CLogger::Get()->Write(FromScheduler, LogNotice, "Index: %d | Weight: %d | Addr: %x", i, tempWeight, m_pTask[i]);
	}
}
