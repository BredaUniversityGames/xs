/* SIE CONFIDENTIAL
 * PlayStation(R)5 Programmer Tool Runtime Library Release 5.00.00.33-00.00.00.0.1
 * Copyright (C) 2019 Sony Interactive Entertainment Inc.
 */

#include <new>
#include <cstdlib>
#include <cstdio>

void *user_new(std::size_t size);
void *user_new(std::size_t size, const std::nothrow_t&) noexcept;
void *user_new_array(std::size_t size);
void *user_new_array(std::size_t size, const std::nothrow_t&) noexcept;
void user_delete(void *ptr) noexcept;
void user_delete(void *ptr, const std::nothrow_t&) noexcept;
void user_delete_array(void *ptr) throw();
void user_delete_array(void *ptr, const std::nothrow_t&) noexcept;
#if __cplusplus >= 201402L
void user_delete(void *ptr, std::size_t) noexcept;
void user_delete(void *ptr, std::size_t, const std::nothrow_t& x) noexcept;
void user_delete_array(void *ptr, std::size_t) noexcept;
void user_delete_array(void *ptr, std::size_t, const std::nothrow_t& x) noexcept;
#endif
#if __cplusplus >= 201703L
void *user_new(std::size_t size, std::align_val_t align);
void *user_new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept;
void *user_new_array(std::size_t size, std::align_val_t align);
void *user_new_array(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept;
void user_delete(void *ptr, std::align_val_t align) noexcept;
void user_delete(void *ptr, std::size_t, std::align_val_t align) noexcept;
void user_delete(void *ptr, std::align_val_t align, const std::nothrow_t&) noexcept;
void user_delete_array(void *ptr, std::align_val_t align) noexcept;
void user_delete_array(void *ptr, std::size_t, std::align_val_t align) noexcept;
void user_delete_array(void *ptr, std::align_val_t align, const std::nothrow_t&) noexcept;
#endif

//E Replace operator new.
//J operator new と置き換わる
void *user_new(std::size_t size)
{
	void *ptr;

	if (size == 0)
		size = 1;

	while ((ptr = (void *)std::malloc(size)) == NULL) {
		//E Obtain new_handler
		//J new_handler を取得する
		std::new_handler handler = std::get_new_handler();

		//E When new_handler is a NULL pointer, bad_alloc is send. If not, new_handler is called.
		//J new_handler が NULL ポインタの場合、bad_alloc を送出する、そうでない場合、new_handler を呼び出す
		if (!handler)
			throw std::bad_alloc();
		else
			(*handler)();
	}
	return ptr;
}

//E Replace operator new(std::nothrow).
//J operator(std::nothrow) と置き換わる
void *user_new(std::size_t size, const std::nothrow_t&) noexcept
{
	void *ptr;

	try {
		ptr = user_new(size);
	}
	catch (...) {
		ptr = NULL;
	}

	return ptr;
}

//E Replace operator new[].
//J operator new[] と置き換わる
void *user_new_array(std::size_t size)
{
	return user_new(size);
}

//E Replace operator new[](std::nothrow).
//J operator new[](std::nothrow) と置き換わる
void *user_new_array(std::size_t size, const std::nothrow_t&) noexcept
{
	void *ptr;

	try {
		ptr = user_new_array(size);
	}
	catch (...) {
		ptr = NULL;
	}

	return ptr;
}

//E Replace operator delete.
//J operator delete と置き換わる
void user_delete(void *ptr) noexcept
{
	//E In the case of the NULL pointer, no action will be taken.
	//J NULL ポインタの場合、何も行わない
	if (ptr != NULL)
		std::free(ptr);
}

//E Replace operator delete(std::nothrow).
//J operator delete(std::nothrow) と置き換わる
void user_delete(void *ptr, const std::nothrow_t&) noexcept
{
	user_delete(ptr);
}

//E Replace operator delete[].
//J operator delete[] と置き換わる
void user_delete_array(void *ptr) noexcept
{
	user_delete(ptr);
}

//E Replace operator delete[](std::nothrow).
//J operator delete[](std::nothrow) と置き換わる
void user_delete_array(void *ptr, const std::nothrow_t&) noexcept
{
	user_delete_array(ptr);
}

#if __cplusplus >= 201402L
//E Replace operator delete(std::size_t).
//J operator delete と置き換わる
void user_delete(void *ptr, std::size_t) noexcept
{
	user_delete(ptr);
}

//E Replace operator delete(std::size_t, std::nothrow).
//J operator delete(std::size_t, std::nothrow) と置き換わる
void user_delete(void *ptr, std::size_t, const std::nothrow_t& x) noexcept
{
	user_delete(ptr, x);
}

//E Replace operator delete[](std::size_t).
//J operator delete[](std::size_t) と置き換わる
void user_delete_array(void *ptr, std::size_t) noexcept
{
	user_delete_array(ptr);
}

//E Replace operator delete[](std::size_t, std::nothrow).
//J operator delete[](std::size_t, std::nothrow) と置き換わる
void user_delete_array(void *ptr, std::size_t, const std::nothrow_t& x) noexcept
{
	user_delete_array(ptr, x);
}
#endif

#if __cplusplus >= 201703L
//E Replace operator new(std::align_val_t).
//J operator new(std::align_val_t) と置き換わる
void *user_new(std::size_t size, std::align_val_t align)
{
	void *ptr;
	size_t alignment = (size_t)align;

	if (alignment == 0 || alignment & (alignment - 1))
		std::abort();

	if (size == 0)
		size = 1;

	while ((ptr = (void *)std::aligned_alloc(alignment, size)) == NULL) {
		//E Obtain new_handler
		//J new_handler を取得する
		std::new_handler handler = std::get_new_handler();

		//E When new_handler is a NULL pointer, bad_alloc is send. If not, new_handler is called.
		//J new_handler が NULL ポインタの場合、bad_alloc を送出する、そうでない場合、new_handler を呼び出す
		if (!handler)
			throw std::bad_alloc();
		else
			(*handler)();
	}
	return ptr;
}

//E Replace operator new(std::align_val_t, std::nothrow).
//J operator(std::align_val_t, std::nothrow) と置き換わる
void *user_new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept
{
	void *ptr;

	try {
		ptr = user_new(size, align);
	}
	catch (...) {
		ptr = NULL;
	}

	return ptr;
}

//E Replace operator new[](std::align_val_t).
//J operator new[](std::align_val_t) と置き換わる
void *user_new_array(std::size_t size, std::align_val_t align)
{
	return user_new(size, align);
}

//E Replace operator new[](std::align_val_t, std::nothrow).
//J operator new[](std::align_val_t, std::nothrow) と置き換わる
void *user_new_array(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept
{
	void *ptr;

	try {
		ptr = user_new_array(size, align);
	}
	catch (...) {
		ptr = NULL;
	}

	return ptr;
}

//E Replace operator delete(std::align_val_t).
//J operator delete(std::align_val_t) と置き換わる
void user_delete(void *ptr, std::align_val_t align) noexcept
{
	//E In the case of the NULL pointer, no action will be taken.
	//J NULL ポインタの場合、何も行わない
	if (ptr != NULL)
		std::free(ptr);
}

//E Replace operator delete(std::size_t, std::align_val_t).
//J operator delete(std::size_t, std::align_val_t) と置き換わる
void user_delete(void *ptr, std::size_t, std::align_val_t align) noexcept
{
	user_delete(ptr, align);
}

//E Replace operator delete(std::align_val_t, std::nothrow).
//J operator delete(std::align_val_t, std::nothrow) と置き換わる
void user_delete(void *ptr, std::align_val_t align, const std::nothrow_t&) noexcept
{
	user_delete(ptr, align);
}

//E Replace operator delete[](std::align_val_t).
//J operator delete[](std::align_val_t) と置き換わる
void user_delete_array(void *ptr, std::align_val_t align) noexcept
{
	user_delete(ptr, align);
}

//E Replace operator delete[](std::size_t, std::align_val_t).
//J operator delete[](std::size_t, std::align_val_t) と置き換わる
void user_delete_array(void *ptr, std::size_t, std::align_val_t align) noexcept
{
	user_delete_array(ptr, align);
}

//E Replace operator delete[](std::align_val_t, std::nothrow).
//J operator delete[](std::align_val_t, std::nothrow) と置き換わる
void user_delete_array(void *ptr, std::align_val_t align, const std::nothrow_t&) noexcept
{
	user_delete_array(ptr, align);
}
#endif
