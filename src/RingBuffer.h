#include <iostream>
#include <mutex>
using namespace std;

template<class elem_type>
class RingBuffer
{
public:
    RingBuffer(int size = 100);
    ~RingBuffer();

    bool IsEmpty(void) const { return head == tail; }
    bool IsFull(void) const { return (tail + 1) % size == head; }
    int GetNumUsed(void) const { return (tail - head + size) % size; }

    bool Put(const elem_type &elem);
    bool Get(elem_type &elem);

    void Clear(void) { tail = head = 0; }

private:
    elem_type *buffer;
    int size;
    int head;
    int tail;

    HANDLE mutex;
};

template<class elem_type>
RingBuffer<elem_type>::RingBuffer(int size)
{
    buffer = new elem_type[size];
    this->size = size;
    tail = head = 0;

    mutex = ::CreateMutex(NULL, FALSE, NULL);
}

template<class elem_type>
RingBuffer<elem_type>::~RingBuffer()
{
    delete[] buffer;
    ::CloseHandle(mutex);
}

template<class elem_type>
bool RingBuffer<elem_type>::Put(const elem_type &elem)
{
    if (IsFull())
        return FALSE;

    buffer[tail] = elem;
    tail = (tail + 1) % size;

    return TRUE;
}

template<class elem_type>
bool RingBuffer<elem_type>::Get(elem_type &elem)
{
    WaitForSingleObject(mutex, INFINITE);

    if (IsEmpty())
        return FALSE;

    elem = buffer[head];
    head = (head + 1) % size;

    ::ReleaseMutex(mutex);

    return TRUE;
}
