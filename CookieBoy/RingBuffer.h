#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <memory.h>

template<typename T> 
class RingBuffer
{
public:
	RingBuffer(int size) : Buffer(NULL), Size(0), Start(0), End(0) { Resize(size); }
	~RingBuffer() { delete[] Buffer; }

	int Capacity();
	int Used();
	int Available();
	void Resize(int newSize);
	void Write(T* src, int count);
	void Read(T* dst, int count);

protected:
	T* Buffer;
	int Size;
	int Start;
	int End;
	int used;
};

template<typename T>
void RingBuffer<T>::Resize(int newSize)
{
	if (Buffer)
	{
		delete[] Buffer;
	}

	Size = newSize + 1;
	Buffer = new T[Size];

	Start = 0;
	End = 0;
	used = 0;
}

template<typename T>
int RingBuffer<T>::Capacity()
{
	return Size - 1;
}

template<typename T>
int RingBuffer<T>::Available()
{
	return Size - 1 - used;
}

template<typename T>
int RingBuffer<T>::Used()
{
	return used;
}

template<typename T>
void RingBuffer<T>::Write(T* src, int count)
{
	int startAvail = Available();
	int startUsed = Used();

	int bufferSize = Available();
	if (bufferSize == 0)
	{
		return;
	}

	if (count > bufferSize)
	{
		count = bufferSize;
	}

	if (End == Capacity())
	{
		End = 0;
	}

	if (End + count > Size - 1)
	{
		int num = Size - End - 1;

		memcpy(Buffer + End, src, num * sizeof(T));

		End = 0;
		count -= num;
		src += num;
		used += num;
	}

	memcpy(Buffer + End, src, count * sizeof(T));
	End += count;
	used += count;
	if (End == Size)
	{
		End = 0;
	}
}

template<typename T>
void RingBuffer<T>::Read(T* dst, int count)
{
	int bufferSize = Used();
	if (bufferSize == 0)
	{
		return;
	}

	if (count > bufferSize)
	{
		count = bufferSize;
	}

	if (Start == Capacity())
	{
		Start = 0;
	}

	if (Start + count > Size - 1)
	{
		int num = Size - Start - 1;

		memcpy(dst, Buffer + Start, num * sizeof(T));

		Start = 0;
		count -= num;
		dst += num;
		used -= num;
	}

	memcpy(dst, Buffer + Start, count * sizeof(T));
	Start += count;
	used -= count;
	if (Start == Size)
	{
		Start = 0;
	}
}

#endif