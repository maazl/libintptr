#ifndef INTPTR_H
#define INTPTR_H

#include <atomic>
#include <type_traits>
#include <cassert>
#include <stdint.h>

namespace mmutil
{

class alignas(32) ref_count
{
	friend class int_ptr_base;
	friend class atomic_int_ptr_base;

	std::atomic<uintptr_t> count;

protected:
	ref_count() : count(0) {}
	~ref_count() {}
public:
	ref_count(const ref_count&) = delete;
	ref_count& operator=(const ref_count&) = delete;

	inline long use_count();
};

class int_ptr_base
{
	friend class atomic_int_ptr_base;
public:
	static_assert(sizeof(uintptr_t) >= 4, "Cannot deal with less than 32 bit platform.");
	static constexpr unsigned  alignment = alignof(ref_count);
	static constexpr uintptr_t counter_mask = alignment - 1U;
	static constexpr uintptr_t pointer_mask = ~counter_mask;

protected:
	ref_count* ptr;

	static void acquire(ref_count* ptr) noexcept { if (ptr) ptr->count += int_ptr_base::alignment; }
	static bool release(ref_count* ptr) noexcept { return ptr && (ptr->count -= int_ptr_base::alignment) == 0; }
	void swap(int_ptr_base&& r) noexcept { std::swap(ptr, r.ptr); }
};

inline long ref_count::use_count()
{	return static_cast<long>(count / int_ptr_base::alignment);
}

template <typename T>
class int_ptr : private int_ptr_base
{
	static_assert(std::is_convertible<T*, ref_count*>::value, "Only types implementing ref_count may be used with int_ptr");
	friend class std::atomic<int_ptr<T>>;

	constexpr explicit int_ptr(ref_count* raw) { ptr = raw; }

public:
	typedef T element_type;

	constexpr int_ptr() noexcept { ptr = nullptr; }
	          int_ptr(T* ptr) noexcept { acquire(this->ptr = ptr); }
	          int_ptr(const int_ptr<T>& r) noexcept { acquire(ptr = r.ptr); }
	constexpr int_ptr(int_ptr<T>&& r) noexcept { ptr = r.ptr; r.ptr = nullptr; }
	          ~int_ptr() { if (release(ptr)) delete &static_cast<T&>(*ptr); }
	// Basic operators
	constexpr T* get() const noexcept { return static_cast<T*>(ptr); }
	constexpr explicit operator bool() const noexcept { return ptr != nullptr; }
	constexpr T& operator*() const { assert(ptr); return *get(); }
	constexpr T* operator->() const { assert(ptr); return get(); }
	friend void  std::swap(int_ptr<T>& l, int_ptr<T>& r) noexcept { l.swap(r); }
	void         reset() { this->~int_ptr(); ptr = nullptr; }
	// assignment
	int_ptr<T>&  operator=(T* ptr) { swap(int_ptr<T>(ptr)); return *this; }
	int_ptr<T>&  operator=(const int_ptr<T>& r) { swap(int_ptr<T>(r)); return *this; }
	int_ptr<T>&  operator=(int_ptr<T>&& r) noexcept { swap(std::move(r)); return *this; }
	// manual resource management for adaption of C libraries.
	constexpr T* toCptr() noexcept { T* ret = get(); ptr = nullptr; return ret; }
	static constexpr int_ptr<T> fromCptr(T* ptr) noexcept { return int_ptr<T>(static_cast<ref_count*>(ptr)); }
};

/// Type erasure part of std::atomic<int_ptr<T>>
class atomic_int_ptr_base
{
protected:
	mutable std::atomic<uintptr_t> ptr;

	ref_count* acquire() const noexcept;
	ref_count* reset() noexcept;
	void swap(int_ptr_base& r) noexcept;

	uintptr_t compare_exchange_weak(int_ptr_base& oldval, const int_ptr_base& newval) noexcept;
	uintptr_t compare_exchange_strong(int_ptr_base& oldval, const int_ptr_base& newval) noexcept;

	constexpr explicit atomic_int_ptr_base(ref_count* ptr = nullptr) noexcept : ptr(reinterpret_cast<uintptr_t>(ptr)) {}

public:
#ifndef NDEBUG
	static std::atomic<unsigned> max_outer_count;
#endif
};

}

namespace std
{

template <typename T>
class atomic<mmutil::int_ptr<T>> : private mmutil::atomic_int_ptr_base
{
public:
	typedef mmutil::int_ptr<T> value_type;

private:
	void swap(value_type&& r) noexcept { mmutil::atomic_int_ptr_base::swap(r); }
	/// finalize CAS operation
	/// @param action control action to perform. The result of one of the
	/// atomic_int_ptr_base compare_exchange functions is expected here.<br/>
	/// If the pointer_mask part is non-zero this object needs to be deleted.
	/// The counter_mask part is the return value.
	bool compare_exchange(uintptr_t action)
	{	delete static_cast<T*>(reinterpret_cast<mmutil::ref_count*>(action & mmutil::int_ptr_base::pointer_mask));
		return static_cast<bool>(action & mmutil::int_ptr_base::counter_mask);
	}

public:
	constexpr atomic() noexcept {}
	          atomic(const value_type& r) noexcept : atomic_int_ptr_base(r.ptr) { mmutil::int_ptr_base::acquire(r.ptr); }
	          atomic(const atomic<value_type>& r) = delete;
	constexpr atomic(value_type&& r) noexcept : atomic_int_ptr_base(r.ptr) { r.ptr = nullptr; }
	          ~atomic()
	{	uintptr_t raw = ptr;
		assert((raw & mmutil::int_ptr_base::counter_mask) == 0);
		if (mmutil::int_ptr_base::release(reinterpret_cast<mmutil::ref_count*>(raw)))
			delete &static_cast<T&>(*reinterpret_cast<mmutil::ref_count*>(raw));
	}
	// basic operators
	operator  value_type() const noexcept { return value_type(acquire()); }
	constexpr explicit operator bool() const noexcept { return (ptr & value_type::pointer_mask) != 0; }
	// mutable operators
	void      reset() { delete static_cast<T*>(mmutil::atomic_int_ptr_base::reset()); }
	inline friend void swap(value_type& l, atomic<value_type>& r) noexcept { r.swap(std::move(l)); }
	inline friend void swap(atomic<value_type>& l, value_type& r) noexcept { l.swap(std::move(r)); }
	// assignment
	atomic<value_type>& operator=(const atomic<value_type>&) = delete;
	void      operator=(const value_type& r) { swap(value_type(r)); }
	void      operator=(value_type&& r) noexcept { swap(std::move(r)); }

	bool compare_exchange_weak(value_type& oldval, const value_type& newval)
	{	return compare_exchange(mmutil::atomic_int_ptr_base::compare_exchange_weak(oldval, newval)); }

	bool compare_exchange_strong(value_type& oldval, const value_type& newval)
	{	return compare_exchange(mmutil::atomic_int_ptr_base::compare_exchange_strong(oldval, newval)); }

	bool is_lock_free() const noexcept { return ptr.is_lock_free(); }
#if __cplusplus >= 201703L
	static constexpr bool is_always_lock_free = atomic<uintptr_t>::is_always_lock_free;
#endif
};

}

namespace mmutil
{

template <typename T> using atomic_int_ptr = std::atomic<int_ptr<T>>;

}
#endif
