#include <intptr.h>
#include <cstdio>

using namespace mmutil;
using namespace std;

struct Obj : public ref_count
{ int I;
	Obj(int i) : I(i) { printf("TestObj(%i)::TestObj()\n", I); }
	~Obj() { printf("TestObj(%i)::~TestObj()\n", I); }
};

static int_ptr<Obj> factory(int i)
{
	return int_ptr<Obj>(new Obj(i));
}

static void log(const char* msg, const int_ptr<Obj>& ptr)
{	auto obj = ptr.get();
	if (obj)
		printf("%s -> %i [%li]\n", msg, obj->I, obj->use_count());
	else
		printf("%s -> null\n", msg);
}

int main()
{
	int_ptr<Obj> p0;
	log("int_ptr()", p0);
	int_ptr<Obj> p1(new Obj(1));
	log("int_ptr(T*)", p1);
	int_ptr<Obj> p2(factory(2));
	log("int_ptr(int_ptr<T>&&)", p2);
	int_ptr<Obj> p3(p2);
	log("int_ptr(const int_ptr<T>&)", p3);

	p1 = p2;
	log("operator=(const int_ptr<T>&)", p1);
	p1 = factory(3);
	log("operator=(int_ptr<T>&&)", p1);
	p2 = p1.get();
	log("operator=(T*)", p1);
	p3 = p3;
	log("operator=(self)", p3);

	swap(p0, p1);
	log("swap(int_ptr<T>&&)", p0);
	p2.reset();
	log("reset()", p2);

	Obj* pC = p3.toCptr();
	log("toCptr()", p3);
	p1 = int_ptr<Obj>::fromCptr(pC);
	log("fromCptr(T*)", p1);

	return 0;
}
