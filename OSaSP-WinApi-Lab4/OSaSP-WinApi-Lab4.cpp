// OSaSP-WinApi-Lab4.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include "ConcurrentQueue.h"

struct FileInfo{
	std::string fileName;
	int occurrences;
};

std::string target = "house";
ConcurrentQueue<FileInfo> queue;
CRITICAL_SECTION criticalSection;
CONDITION_VARIABLE conditionVariable;
int workCount = 0;

VOID CALLBACK MyWorkCallback(
	PTP_CALLBACK_INSTANCE Instance,
	PVOID				  Parameter,
	PTP_WORK			  Work
)
{
	std::string line;

	wchar_t* param = (wchar_t*) Parameter;
	std::wstring ws(param);
	std::string fullName(ws.begin(), ws.end());
	int occurrences = 0;
	std::ifstream in(ws);
	if (in.is_open())
	{
		while (getline(in, line))
		{
			std::string::size_type pos = 0;
			while ((pos = line.find(target, pos)) != std::string::npos) {
				++occurrences;
				pos += target.length();
			}
		}
	}
	in.close();
	FileInfo fileInfo;
	fileInfo.fileName = fullName.substr(fullName.find_last_of("/\\") + 1);
	fileInfo.occurrences = occurrences;
	queue.Enqueue(fileInfo);
	
	delete[] param;
	EnterCriticalSection(&criticalSection);
	workCount--;
	WakeAllConditionVariable(&conditionVariable);
	LeaveCriticalSection(&criticalSection);
}

int main()
{
	const LPCTSTR path = L"./TestFiles/*.txt";
	const wchar_t dir[13] = L"./TestFiles/";
	InitializeCriticalSection(&criticalSection);
	InitializeConditionVariable(&conditionVariable);
	std::list<PTP_WORK> works;
	PTP_POOL pool = NULL;
	PTP_WORK_CALLBACK workcallback = MyWorkCallback;
	WIN32_FIND_DATAW fileData;

	HANDLE hFind = FindFirstFile(path, &fileData);
	do
	{
		wchar_t fullPath[280];
		wcscpy_s(fullPath, dir);
		wcscat_s(fullPath, fileData.cFileName);
		
		wchar_t* param = new wchar_t[280];
		wcscpy(param, fullPath);

		PTP_WORK work = CreateThreadpoolWork(workcallback, param, NULL);
		if (work)
		{
			SubmitThreadpoolWork(work);
			EnterCriticalSection(&criticalSection);
			workCount++;
			LeaveCriticalSection(&criticalSection);
		}
		else
		{
			delete[] param;
		}

		
		FindNextFile(hFind, &fileData);
	} while (GetLastError() != ERROR_NO_MORE_FILES);

	EnterCriticalSection(&criticalSection);
	while (workCount != 0)
	{
		SleepConditionVariableCS(&conditionVariable, &criticalSection, INFINITE);
	}
	LeaveCriticalSection(&criticalSection);
	
	while (!queue.IsEmpty())
	{
		FileInfo fileInfo;
		bool result = queue.TryDequeue(fileInfo);
		printf("%s : %d\n", fileInfo.fileName.c_str(), fileInfo.occurrences);
	}
	
	DeleteCriticalSection(&criticalSection);
}