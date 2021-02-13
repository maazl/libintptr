#include <intptr.h>
#include <cstdio>
#include <thread>
#include <chrono>

using namespace mmutil;
using namespace std;
using namespace std::chrono_literals;

static unsigned thread_count = 300;
static auto test_time = 3s;

struct logger
{
#ifndef NDEBUG
	unsigned long value = 0;
	logger() : value(0) {}
	void operator++() { ++value; }
	void print(const char* task, int id) { printf("%s %i: count %lu\n", task, id, value); }
#else
	void operator++() {}
	void print(const char*, int) {}
#endif
};

struct Obj : public ref_count
{ int I;
	Obj(int i) : I(i) { printf("TestObj(%i)::TestObj()\n", I); }
	~Obj() { printf("TestObj(%i)::~TestObj()\n", I); }
};

static bool terminate_test = false;
static atomic<int_ptr<Obj>> instance;

static void writer(int id)
{
	int_ptr<Obj> myObj = new Obj(id);

	logger count;
	do
	{	instance = myObj;
		++count;
	} while (!terminate_test);

	count.print("writer", id);
}

static void reader(int id)
{
	int_ptr<Obj> myObj;

	logger count;
	do
	{	myObj = instance;
		++count;
	} while (!terminate_test);

	count.print("reader", id);
}

static void readwrite(int id)
{
	int_ptr<Obj> myObj = new Obj(id);

	logger count;
	do
	{	instance = myObj;
		myObj = instance;
		++count;
	} while (!terminate_test);

	count.print("readwrite", id);
}

int main()
{
	std::thread threads[thread_count];

	void (*funcs[3])(int) =
	{	writer,
		reader,
		readwrite
	};

	for (unsigned id = 0; id < thread_count; ++id)
		threads[id] = thread(funcs[id%3], id);

	this_thread::sleep_for(test_time);
	terminate_test = true;

	for (unsigned id = 0; id < thread_count; ++id)
		threads[id].join();

	#ifndef NDEBUG
	printf ("max_outer_count: %u\n", atomic_int_ptr_base::max_outer_count.load());
	#endif

	return 0;
}
