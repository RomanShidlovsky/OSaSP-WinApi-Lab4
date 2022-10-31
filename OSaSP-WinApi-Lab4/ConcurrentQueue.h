#include <list>
#include <assert.h>
#include <threadpoolapiset.h>

template <typename T>
class ConcurrentQueue
{
public:
	ConcurrentQueue(int nCapacity = -1)
	{
		m_nCapacity = nCapacity;
		InitializeCriticalSection(&m_lock);
		m_queue_not_empty = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_queue_not_full = CreateEvent(NULL, FALSE, TRUE, NULL);
	}
	virtual ~ConcurrentQueue()
	{
		m_list.clear();
		CloseHandle(m_queue_not_empty);
		CloseHandle(m_queue_not_full);
		DeleteCriticalSection(&m_lock);
	}
	virtual bool TryEnqueue(T element)
	{
		if (m_nCapacity <= 0 || m_list.size() < m_nCapacity)
		{
			EnterCriticalSection(&m_lock);
			m_list.push_back(element);
			SetEvent(m_queue_not_empty);
			LeaveCriticalSection(&m_lock);
			return true;
		}
		return false;
	}

	virtual void Enqueue(T element)
	{
		if (m_nCapacity <= 0)
		{
			EnterCriticalSection(&m_lock);
			m_list.push_back(element);
			SetEvent(m_queue_not_empty);
			LeaveCriticalSection(&m_lock);
			return;
		}

		bool added = false;
		while (!added)
		{
			if (WaitForSingleObject(m_queue_not_full, INFINITE) != WAIT_OBJECT_0)
			{
				assert(FALSE);
			}

			EnterCriticalSection(&m_lock);
			if (m_list.size() < m_nCapacity)
			{
				m_list.push_back(element);
				added = true;
				SetEvent(m_queue_not_empty);
				if (m_list.size() < m_nCapacity)
				{
					SetEvent(m_queue_not_full);
				}
			}
			LeaveCriticalSection(&m_lock);
		}
	}

	virtual bool TryDequeue(T& out)
	{
		if (!m_list.empty())
		{
			EnterCriticalSection(&m_lock);
			out = m_list.front();
			m_list.pop_front();
			SetEvent(m_queue_not_full);
			if (m_list.empty())
			{
				SetEvent(m_queue_not_empty);
			}
			LeaveCriticalSection(&m_lock);
			return true;
		}
		return false;
	}

	virtual void Dequeue(T& out)
	{
		bool done = false;
		while (!done)
		{
			if (WaitForSingleObject(m_queue_not_empty, INFINITE) != WAIT_OBJECT_0)
			{
				assert(false);
			}

			EnterCriticalSection(&m_lock);
			if (!m_list.empty())
			{
				out = m_list.front();
				m_list.pop_front();
				done = true;
				if (!m_list.empty())
				{
					SetEvent(m_queue_not_empty);
				}
			}
			LeaveCriticalSection(&m_lock);
		}
	}

	virtual bool TryPeek(T& out)
	{
		if (!m_list.empty())
		{
			EnterCriticalSection(&m_lock);
			out = m_list.front();
			LeaveCriticalSection(&m_lock);
			return true;
		}
		return false;
	}

	virtual bool IsEmpty()
	{
		return m_list.empty();
	}
protected:
	int m_nCapacity;
	std::list<T> m_list;

	CRITICAL_SECTION m_lock;
	HANDLE m_queue_not_empty;
	HANDLE m_queue_not_full;
};