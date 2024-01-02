#include <windows.h>
#include <iostream>

int n, minV, maxV, ave;

CRITICAL_SECTION cs;

DWORD WINAPI min_max(LPVOID iNum)
{
    int* arr = (int*)iNum;
    int min = INT_MAX;
    int max = INT_MIN;
    for (int i = 0; i < n; i++)
    {
        if (arr[i] < min)
        {
            min = arr[i];
        }
        Sleep(7);
        if (arr[i] > max)
        {
            max = arr[i];
        }
        Sleep(7);
    }
    minV = min;
    maxV = max;

    EnterCriticalSection(&cs);
    std::cout << "Min element: " << min << "\nMax element: " << max << "\n";
    LeaveCriticalSection(&cs);

    return 0;
}

DWORD WINAPI average(LPVOID iNum)
{
    int* arr = (int*)iNum;
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += arr[i];
        Sleep(12);
    }
    sum /= n;
    ave = sum;

    EnterCriticalSection(&cs);
    std::cout << "Average: " << sum << "\n";
    LeaveCriticalSection(&cs);

    return 0;
}

int main()
{
    std::cout << "Enter the number of array elements: ";
    std::cin >> n;
    int* arr = new int[n];

    std::cout << "Enter the array elements: ";
    for (int i = 0; i < n; i++)
    {
        std::cin >> arr[i];
    }

    std::cout << "Your array: ";
    for (int i = 0; i < n; i++)
    {
        std::cout << arr[i] << " ";
    }
    std::cout << "\n";

    InitializeCriticalSection(&cs);

    HANDLE hThread1;
    DWORD IDThread1;

    hThread1 = CreateThread(NULL, 0, min_max, (void*)arr, 0, &IDThread1);
    if (hThread1 == NULL)
        return GetLastError();

    HANDLE hThread2;
    DWORD IDThread2;

    hThread2 = CreateThread(NULL, 0, average, (void*)arr, 0, &IDThread2);
    if (hThread2 == NULL)
        return GetLastError();

    //waiting for the hThread1 finish working
    WaitForSingleObject(hThread1, INFINITE);
    WaitForSingleObject(hThread2, INFINITE);

    // closing the stream descriptor
    CloseHandle(hThread2);
    CloseHandle(hThread1);

    for (int i = 0; i < n; i++)
    {
        if (arr[i] == minV || arr[i] == maxV)
        {
            arr[i] = ave;
        }
    }

    std::cout << "New array:\n";
    for (int i = 0; i < n; i++)
    {
        std::cout << arr[i] << " ";
    }
    std::cout << "\n";

    DeleteCriticalSection(&cs);

    return 0;
}
