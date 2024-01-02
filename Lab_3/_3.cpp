#include <iostream>
#include "windows.h"

int* arr;
int arr_size;
HANDLE* stopEvents;
HANDLE* continueEvents;
int num;
bool* isWorking;
CRITICAL_SECTION cs;

void resetEvents(HANDLE* events)
{
    for (int i = 0; i < num; i++)
    {
        ResetEvent(events[i]);
    }
}

void showArray()
{
    for (int i = 0; i < arr_size; i++)
    {
        std::cout << arr[i] << " ";
    }
    std::cout << "\n";
}

DWORD WINAPI marker(LPVOID v)
{
    auto number = (int)v;
    int markedElements = 0;
    srand(number);
    bool* visitedArray = new bool[arr_size];
    for (int i = 0; i < arr_size; i++)
    {
        visitedArray[i] = false;
    }
    while (true)
    {
        markedElements = 0;
        while (true)
        {
            int randomNum = rand() % arr_size;
            if (arr[randomNum] != 0)
            {
                EnterCriticalSection(&cs);
                std::cout << "Thread number = " << number + 1 <<
                    ". Amount of marked elements = " << markedElements <<
                    ". Index of an array element that cannot be marked: " << randomNum + 1 << "\n";
                SetEvent(stopEvents[number]);
                LeaveCriticalSection(&cs);
                break;
            }
            EnterCriticalSection(&cs);
            if (arr[randomNum] == 0)
            {
                Sleep(5);
                arr[randomNum] = number + 1;
                markedElements++;
                visitedArray[randomNum] = true;
                Sleep(5);
            }
            LeaveCriticalSection(&cs);
        }
        WaitForSingleObject(continueEvents[number], INFINITE);
        ResetEvent(continueEvents[number]);
        if (!isWorking[number])
        {
            EnterCriticalSection(&cs);
            std::cout << number + 1 << " closing\n";
            for (int i = 0; i < arr_size; i++)
            {
                if (visitedArray[i])
                {
                    arr[i] = 0;
                }
            }
            SetEvent(stopEvents[number]);
            LeaveCriticalSection(&cs);
            return 0;
        }
    }
}

int main()
{
    std::cout << "Enter the number of array elements:\n";
    std::cin >> arr_size;
    arr = new int[arr_size];
    for (int i = 0; i < arr_size; i++)
    {
        arr[i] = 0;
    }
    std::cout << "Enter the number of threads:\n";
    std::cin >> num;
    HANDLE* threads = new HANDLE[num];
    stopEvents = new HANDLE[num];
    continueEvents = new HANDLE[num];
    isWorking = new bool[num];
    InitializeCriticalSection(&cs);
    for (int i = 0; i < num; i++)
    {
        stopEvents[i] = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL);
        continueEvents[i] = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL);
        threads[i] = CreateThread(NULL, 0, marker, (void*)i, 0, NULL);
        isWorking[i] = true;
    }
    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num; j++)
        {
            if (isWorking[j])
            {
                ResetEvent(stopEvents[j]);
            }
        }
        WaitForMultipleObjects(num, stopEvents, TRUE, INFINITE);
        int numToStop;
        std::cout << "Array:\n";
        showArray();
        while (true)
        {
            std::cout << "Enter number of thread to stop between 1 and " << num << ". Already stopped threads: ";
            for (int j = 0; j < num; j++)
            {
                if (!isWorking[j])
                {
                    std::cout << j + 1 << " ";
                }
            }
            std::cout << "\n";
            std::cin >> numToStop;
            numToStop--;
            if (numToStop >= 0 && numToStop < num && isWorking[numToStop])
            {
                isWorking[numToStop] = FALSE;
                ResetEvent(stopEvents[numToStop]);
                SetEvent(continueEvents[numToStop]);
                WaitForSingleObject(stopEvents[numToStop], INFINITE);
                break;
            }
            else
            {
                std::cout << "Incorrect number, try again!\n";
            }
        }
        for (int j = 0; j < num; j++)
        {
            if (isWorking[j])
            {
                SetEvent(continueEvents[j]);
            }
        }
    }
}