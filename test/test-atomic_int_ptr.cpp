#include <intptr.h>
#include <cstdio>

using namespace mmutil;
using namespace std;

class Obj : public ref_count
{
#ifndef NDEBUG
	void log(const char* meth) { printf("TestObj(%p:%i)::%s\n", (ref_count*)this, I, meth); }
#else
	void log(const char* meth) { printf("TestObj(%i)::%s\n", I, meth); }
#endif
public:
	int I;
	Obj(int i) : I(i) { log("TestObj()"); }
	~Obj() { log("~TestObj()"); }
};

static void log(const char* msg, const int_ptr<Obj>& ptr)
{	auto obj = ptr.get();
	if (obj)
		printf("%s -> %i [%li]\n", msg, obj->I, obj->use_count());
	else
		printf("%s -> null\n", msg);
}

int main()
{
	atomic<int_ptr<Obj>> ap0;
	log("atomic_int_ptr()", ap0);
	int_ptr<Obj> p1(new Obj(1));
	log("int_ptr(T*)", p1);
	atomic<int_ptr<Obj>> ap2(p1);
	log("atomic_int_ptr(const int_ptr<T>&)", ap2);
	atomic<int_ptr<Obj>> ap3(new Obj(2));
	log("atomic_int_ptr(int_ptr<T>&&)", ap3);

	swap(ap3, p1);
	log("swap(atomic_int_ptr<T>&, int_ptr<T>&)", ap3);
	swap(p1, ap3);
	log("swap(int_ptr<T>&, atomic_int_ptr<T>&)", ap3);
	ap3.reset();
	log("reset()", ap3);

	ap0 = p1;
	log("operator=(const int_ptr<T>&)", ap0);
	ap0 = p1;
	log("operator=(self)", ap0);
	ap0 = new Obj(3);
	log("operator=(int_ptr<T>&&)", ap0);

	bool ret = ap0.compare_exchange_strong(p1, new Obj(4));
	log("compare_exchange_strong()", ap0);
	log(" oldval", p1);
	printf(" result %i\n", ret);
	ret = ap0.compare_exchange_strong(p1, new Obj(5));
	log("compare_exchange_strong()", ap0);
	log(" oldval", p1);
	printf(" result %i\n", ret);

	return 0;
}
