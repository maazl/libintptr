#include <intptr.h>

using namespace mmutil;

#ifndef NDEBUG
std::atomic<unsigned> atomic_int_ptr_base::max_outer_count(1);
#endif

ref_count* atomic_int_ptr_base::acquire() const noexcept
{	__builtin_prefetch(reinterpret_cast<ref_count*>(ptr & int_ptr_base::pointer_mask), 1);
	uintptr_t old_outer = ++ptr;
	const uintptr_t outer_count = old_outer & int_ptr_base::counter_mask;
	assert(outer_count != 0); // overflow condition
	ref_count* const new_outer = reinterpret_cast<ref_count*>(old_outer & int_ptr_base::pointer_mask);
	if (new_outer)
		// Transfer counter to obj->count.
		new_outer->count += int_ptr_base::alignment + 1 - outer_count;
	// And reset it in *this.
	if (!ptr.compare_exchange_strong(old_outer, reinterpret_cast<uintptr_t>(new_outer)) && new_outer)
		// Someone else does the job already => undo.
		new_outer->count += outer_count;
		// The global count cannot return to zero here, because we have an active reference.
	#ifndef NDEBUG
	unsigned max_outer = max_outer_count;
	while (max_outer < outer_count
		&& !max_outer_count.compare_exchange_weak(max_outer, static_cast<unsigned>(outer_count)));
	#endif
	return new_outer;
}

ref_count* atomic_int_ptr_base::reset() noexcept
{	uintptr_t old = ptr.exchange(0U);
	ref_count* obj = reinterpret_cast<ref_count*>(old & int_ptr_base::pointer_mask);
	return obj && (obj->count -= (old & int_ptr_base::counter_mask) + int_ptr_base::alignment) == 0
		? obj : nullptr;
}

void atomic_int_ptr_base::swap(int_ptr_base& r) noexcept
{ uintptr_t old = ptr.exchange(reinterpret_cast<uintptr_t>(r.ptr));
  // Transfer hold count to the main counter and get the data with hold count 0 into r.
	uintptr_t outer = old & int_ptr_base::counter_mask;
	if (outer && (old &= int_ptr_base::pointer_mask) != 0U)
		reinterpret_cast<ref_count*>(old)->count -= outer;
	r.ptr = reinterpret_cast<ref_count*>(old);
}

uintptr_t atomic_int_ptr_base::compare_exchange_weak(int_ptr_base& oldval, const int_ptr_base& newval) noexcept
{	uintptr_t preval = reinterpret_cast<uintptr_t>(oldval.ptr);
	if (!ptr.compare_exchange_weak(preval, reinterpret_cast<uintptr_t>(newval.ptr)))
	{	preval = reinterpret_cast<uintptr_t>(oldval.ptr);
		oldval.ptr = acquire();
		return preval; // failed
	}
	// success => update reference counts.
	int_ptr_base::acquire(newval.ptr);
	return int_ptr_base::release(oldval.ptr) * preval + 1;
}

uintptr_t atomic_int_ptr_base::compare_exchange_strong(int_ptr_base& oldval, const int_ptr_base& newval) noexcept
{	uintptr_t preval = reinterpret_cast<uintptr_t>(oldval.ptr);
	while (!ptr.compare_exchange_weak(preval, reinterpret_cast<uintptr_t>(newval.ptr)))
		if ((preval ^ reinterpret_cast<uintptr_t>(oldval.ptr)) > int_ptr_base::counter_mask) // compare pointer part only
		{	preval = reinterpret_cast<uintptr_t>(oldval.ptr);
			ref_count* curptr = acquire();
			if (curptr != oldval.ptr)
			{	oldval.ptr = curptr;
				return preval;
			}
			// retrieved pointer happens to match oldval now => undo acquire and retry
			bool del = int_ptr_base::release(curptr);
			// since there is at least one additional strong reference in oldval the release operation must not cause count 0.
			assert(!del);
		}
	// success => update reference counts.
	int_ptr_base::acquire(newval.ptr);
	// release oldval and transfer the outer count if any
	return (oldval.ptr &&
			(oldval.ptr->count -= int_ptr_base::alignment + (preval & int_ptr_base::counter_mask)) == 0)
		* reinterpret_cast<uintptr_t>(oldval.ptr) + 1;
}
