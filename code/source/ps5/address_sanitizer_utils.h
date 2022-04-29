/* SIE CONFIDENTIAL
 * PlayStation(R)5 Programmer Tool Runtime Library Release 5.00.00.33-00.00.00.0.1
 * Copyright (C) 2019 Sony Interactive Entertainment Inc.
 */

#include <kernel.h>
#include <sceerror.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>

#ifdef __has_feature
#if __has_feature(address_sanitizer)
#ifndef ENABLE_ASAN
#define ENABLE_ASAN
#endif
#endif
#endif

#ifdef ENABLE_ASAN
//E Add 32B to every request since malloc-returned memory needs to be aligned at the highest requirement (AVX 256bit)
//J mallocが返すメモリのアライメント要求（AVX 256bit）により、全ての要求サイズに32バイトを追加する
#define ALLOC_OFFSET ((size_t)32)

//E Change to 0 and use the button to "touch allocator metadata" to see that an error is not produced.
//J 0に変更して"touch allocator metadata"ボタンを使用するとエラーが起きないことが確認できる
#define POISON_METADATA 1

//E Metadata to be stored alongside allocated memory.
//J 割り付けたメモリと一緒に格納されるメターデータ
struct Metadata {
	size_t m_size;
	void *m_originalPtr;
};

//E Utility class to unpoison/poison memory for metadata (not thread-safe).
//J メタデータのためのメモリを有害化/無害化するユーティリティクラス（スレッドセーフではない）
struct MetadataView {
	Metadata *m_metadata;

	//E Make the metadata readable by unpoisoning.
	//J メタデータを無害化して読めるようにする
	MetadataView(void* p) : m_metadata(((Metadata*)p - 1)) {
		if (POISON_METADATA)
			ASAN_UNPOISON_MEMORY_REGION(m_metadata, sizeof(Metadata));
	}
	//E Make sure to re-poison when going out of scope.
	//J スコープ外に出るときに確実に再度有害化するようにする
	~MetadataView() {
		if (POISON_METADATA)
			ASAN_POISON_MEMORY_REGION(m_metadata, sizeof(Metadata));
	}

	Metadata &getMetadata() {
		return *m_metadata;
	}
};
#endif
