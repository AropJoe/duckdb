//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/common/serializer/format_serializer.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/field_writer.hpp"
#include "duckdb/common/serializer.hpp"
#include "duckdb/common/serializer/enum_serializer.hpp"
#include "duckdb/common/serializer/serialization_traits.hpp"
#include "duckdb/common/types/interval.hpp"
#include "duckdb/common/types/string_type.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/unordered_set.hpp"

namespace duckdb {

class FormatSerializer {
protected:
	bool serialize_enum_as_string = false;

public:
	// Serialize a value
	template <class T>
	typename std::enable_if<!std::is_enum<T>::value, void>::type WriteProperty(const char *tag, T &value) {
		SetTag(tag);
		WriteValue(value);
	}

	// Serialize a range
	template <class T>
	typename std::enable_if<std::is_pointer<T>::value, void>::type WriteProperty(const char *tag, const T start,
	                                                                             idx_t count) {
		SetTag(tag);
		WriteValue(start, count);
	}

	// Serialize an enum
	template <class T>
	typename std::enable_if<std::is_enum<T>::value, void>::type WriteProperty(const char *tag, T value) {
		SetTag(tag);
		if (serialize_enum_as_string) {
			// Use the enum serializer to lookup tostring function
			auto str = EnumSerializer::EnumToString(value);
			WriteValue(str);
		} else {
			// Use the underlying type
			WriteValue(static_cast<typename std::underlying_type<T>::type>(value));
		}
	}

	// Optional pointer
	template <class T>
	void WriteOptionalProperty(const char *tag, T *ptr) {
		SetTag(tag);
		if (ptr == nullptr) {
			OnOptionalBegin(false);
			OnOptionalEnd(false);
		} else {
			OnOptionalBegin(true);
			WriteValue(*ptr);
			OnOptionalEnd(true);
		}
	}

	// Optional unique_ptr
	template <class T>
	void WriteOptionalProperty(const char *tag, const unique_ptr<T> &ptr) {
		SetTag(tag);
		if (ptr == nullptr) {
			OnOptionalBegin(false);
			OnOptionalEnd(false);
		} else {
			OnOptionalBegin(true);
			WriteValue(*ptr);
			OnOptionalEnd(true);
		}
	}

protected:
	// Unique Pointer Ref
	template <typename T>
	void WriteValue(const unique_ptr<T> &ptr) {
		WriteValue(ptr.get());
	}

	// Pointer
	template <typename T>
	typename std::enable_if<std::is_pointer<T>::value, void>::type WriteValue(const T ptr) {
		if (ptr == nullptr) {
			WriteNull();
		} else {
			WriteValue(*ptr);
		}
	}

	// data_ptr_t
	void WriteValue(data_ptr_t ptr, idx_t count) {
		OnListBegin(count);
		auto end = ptr + count;
		while (ptr != end) {
			WriteValue(*ptr);
			ptr++;
		}
		OnListEnd(count);
	}

	void WriteValue(const_data_ptr_t ptr, idx_t count) {
		OnListBegin(count);
		auto end = ptr + count;
		while (ptr != end) {
			WriteValue(*ptr);
			ptr++;
		}
		OnListEnd(count);
	}

	// Pair
	template <class K, class V>
	void WriteValue(const std::pair<K, V> &pair) {
		OnPairBegin();
		OnPairKeyBegin();
		WriteValue(pair.first);
		OnPairKeyEnd();
		OnPairValueBegin();
		WriteValue(pair.second);
		OnPairValueEnd();
		OnPairEnd();
	}

	// Vector
	template <class T>
	void WriteValue(const vector<T> &vec) {
		auto count = vec.size();
		OnListBegin(count);
		for (auto &item : vec) {
			WriteValue(item);
		}
		OnListEnd(count);
	}

	// UnorderedSet
	// Serialized the same way as a list/vector
	template <class T, class HASH, class CMP>
	void WriteValue(const unordered_set<T, HASH, CMP> &set) {
		auto count = set.size();
		OnListBegin(count);
		for (auto &item : set) {
			WriteValue(item);
		}
		OnListEnd(count);
	}

	// Set
	// Serialized the same way as a list/vector
	template <class T, class HASH, class CMP>
	void WriteValue(const set<T, HASH, CMP> &set) {
		auto count = set.size();
		OnListBegin(count);
		for (auto &item : set) {
			WriteValue(item);
		}
		OnListEnd(count);
	}

	// Map
	template <class K, class V, class HASH, class CMP>
	void WriteValue(const std::unordered_map<K, V, HASH, CMP> &map) {
		auto count = map.size();
		OnMapBegin(count);
		for (auto &item : map) {
			OnMapEntryBegin();
			OnMapKeyBegin();
			WriteValue(item.first);
			OnMapKeyEnd();
			OnMapValueBegin();
			WriteValue(item.second);
			OnMapValueEnd();
			OnMapEntryEnd();
		}
		OnMapEnd(count);
	}

	// class or struct implementing `FormatSerialize(FormatSerializer& FormatSerializer)`;
	template <typename T>
	typename std::enable_if<has_serialize_v<T>(), void>::type WriteValue(T &value) {
		// Else, we defer to the .FormatSerialize method
		OnObjectBegin();
		value.FormatSerialize(*this);
		OnObjectEnd();
	}

	// Handle setting a "tag" (optional)
	virtual void SetTag(const char *tag) {
		(void)tag;
	}

	// Hooks for subclasses to override to implement custom behavior
	virtual void OnListBegin(idx_t count) {
		(void)count;
	}
	virtual void OnListEnd(idx_t count) {
		(void)count;
	}
	virtual void OnMapBegin(idx_t count) {
		(void)count;
	}
	virtual void OnMapEnd(idx_t count) {
		(void)count;
	}
	virtual void OnMapEntryBegin() {
	}
	virtual void OnMapEntryEnd() {
	}
	virtual void OnMapKeyBegin() {
	}
	virtual void OnMapKeyEnd() {
	}
	virtual void OnMapValueBegin() {
	}
	virtual void OnMapValueEnd() {
	}
	virtual void OnOptionalBegin(bool present) {
	}
	virtual void OnOptionalEnd(bool present) {
	}
	virtual void OnObjectBegin() {
	}
	virtual void OnObjectEnd() {
	}
	virtual void OnPairBegin() {
	}
	virtual void OnPairKeyBegin() {
	}
	virtual void OnPairKeyEnd() {
	}
	virtual void OnPairValueBegin() {
	}
	virtual void OnPairValueEnd() {
	}
	virtual void OnPairEnd() {
	}

	// Handle primitive types, a serializer needs to implement these.
	virtual void WriteNull() = 0;
	virtual void WriteValue(uint8_t value) = 0;
	virtual void WriteValue(int8_t value) = 0;
	virtual void WriteValue(uint16_t value) = 0;
	virtual void WriteValue(int16_t value) = 0;
	virtual void WriteValue(uint32_t value) = 0;
	virtual void WriteValue(int32_t value) = 0;
	virtual void WriteValue(uint64_t value) = 0;
	virtual void WriteValue(int64_t value) = 0;
	virtual void WriteValue(hugeint_t value) = 0;
	virtual void WriteValue(float value) = 0;
	virtual void WriteValue(double value) = 0;
	virtual void WriteValue(interval_t value) = 0;
	virtual void WriteValue(const string &value) = 0;
	virtual void WriteValue(const string_t value) = 0;
	virtual void WriteValue(const char *value) = 0;
	virtual void WriteValue(bool value) = 0;
};

} // namespace duckdb
