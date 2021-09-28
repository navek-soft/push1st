#include "cjson.h"
#include <memory>
#include "sstream"

//static inline const size_t map_type_id{ typeid(map_t).hash_code() };
static inline const size_t umapss_type_id{ typeid(std::unordered_map<std::string,std::string>).hash_code() };
static inline const size_t umap_type_id{ typeid(cjson::object_t).hash_code() };
static inline const size_t array_type_id{ typeid(cjson::array_t).hash_code() };
static inline const size_t arrays_type_id{ typeid(std::deque<std::string>).hash_code() };
static inline const size_t cchars_type_id{ typeid(const char*).hash_code() };
static inline const size_t string_type_id{ typeid(std::string).hash_code() };
static inline const size_t stringv_type_id{ typeid(std::string_view).hash_code() };
static inline const size_t nullptr_type_id{ typeid(nullptr).hash_code() };
static inline const size_t bool_type_id{ typeid(bool).hash_code() };
static inline const size_t int_type_id{ typeid(int).hash_code() };
static inline const size_t uint_type_id{ typeid(uint).hash_code() };
static inline const size_t ssize_type_id{ typeid(ssize_t).hash_code() };
static inline const size_t size_type_id{ typeid(size_t).hash_code() };
static inline const size_t float_type_id{ typeid(float).hash_code() };
static inline const size_t double_type_id{ typeid(double).hash_code() };
static inline const size_t jsonval_type_id{ typeid(Json::Value).hash_code() };

bool cjson::unserialize(const std::string_view& document, value_t& value, std::string& err) {
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	return reader->parse(document.data(), document.data() + document.length(), &value, &err);
}

template<typename T>
static inline const T& cast(const std::any& v) { return std::any_cast<const T&>(v); }

static inline std::stringstream& serialize_v(std::stringstream& s, const std::any& val);
static inline void serialize_v(Json::Value& s, const std::any& val);

static inline std::stringstream& serialize_c(std::stringstream& s, const cjson::array_t& val) {
	s << "[";
	if (!val.empty()) {
		for (auto&& v : val) { serialize_v(s, v) << ","; }
		s.seekp(-1, s.cur);
	}
	s << "]";
	return s;
}
/*
static inline std::stringstream& serialize_c(std::stringstream& s, const map_t& val) {
	s << "{";
	if (!val.empty()) {
		for (auto&& v : val) { s << "\"" << v.first << "\":"; serialize_v(s, v.second) << ","; }
		s.seekp(-1, s.cur);
	}
	s << "}";
	return s;
}
*/

static inline std::stringstream& serialize_c(std::stringstream& s, const cjson::object_t& val) {
	s << "{";
	if (!val.empty()) {
		for (auto&& v : val) { s << "\"" << v.first << "\":"; serialize_v(s, v.second) << ","; }
		s.seekp(-1, s.cur);
	}
	s << "}";
	return s;
}

static inline void serialize_c(Json::Value& s, const cjson::array_t& val) {
	auto&& vec{ s.append(Json::arrayValue) };
	if (!val.empty()) {
		for (auto&& v : val) { serialize_v(vec.append(Json::nullValue), v); }
	}
}
/*
static inline void serialize_c(Json::Value& s, const map_t& val) {
	auto&& obj{ s.append(Json::objectValue) };
	if (!val.empty()) {
		for (auto&& v : val) { serialize_v(obj[v.first], v.second); }
	}
}
*/

static inline void serialize_c(Json::Value& s, const cjson::object_t& val) {
	auto&& obj{ s.append(Json::objectValue) };
	if (!val.empty()) {
		for (auto&& v : val) { serialize_v(obj[v.first], v.second); }
	}
}

static inline std::stringstream& serialize_c(std::stringstream& s, const std::unordered_map<std::string, std::string>& val) {
	s << "{";
	if (!val.empty()) {
		for (auto&& v : val) { s << "\"" << v.first << "\":\"" << v.second << "\","; }
		s.seekp(-1, s.cur);
	}
	s << "}";
	return s;
}
static inline void serialize_c(Json::Value& s, const std::unordered_map<std::string, std::string>& val) {
	auto&& obj{ s.append(Json::objectValue) };
	if (!val.empty()) {
		for (auto&& v : val) { obj[v.first] = v.second; }
	}
}

static inline std::stringstream& serialize_c(std::stringstream& s, const std::deque<std::string>& val) {
	s << "[";
	if (!val.empty()) {
		for (auto&& v : val) { s << "\"" << v << "\","; }
		s.seekp(-1, s.cur);
	}
	s << "]";
	return s;
}
static inline void serialize_c(Json::Value& s, const std::deque<std::string>& val) {
	auto&& obj{ s.append(Json::arrayValue) };
	if (!val.empty()) {
		for (auto&& v : val) { obj.append(v); }
	}
}

std::stringstream& serialize_v(std::stringstream& s, const std::any& val) {
	if (val.type().hash_code() == string_type_id) { s << Json::valueToQuotedString(cast<std::string>(val).c_str()); }
//	else if (val.type().hash_code() == map_type_id) { serialize_c(s, cast<map_t>(val)); }
	else if (val.type().hash_code() == umap_type_id) { serialize_c(s, cast<cjson::object_t>(val)); }
	else if (val.type().hash_code() == umapss_type_id) { serialize_c(s, cast<cjson::object_t>(val)); }
	else if (val.type().hash_code() == array_type_id) { serialize_c(s, cast<cjson::array_t>(val)); }
	else if (val.type().hash_code() == stringv_type_id) { s << Json::valueToQuotedString(std::string{ cast<std::string_view>(val) }.c_str()); }
	else if (val.type().hash_code() == cchars_type_id) { s << Json::valueToQuotedString(cast<const char*>(val)); }
	else if (val.type().hash_code() == nullptr_type_id) { s << "null"; }
	else if (val.type().hash_code() == bool_type_id) { s << cast<bool>(val); }
	else if (val.type().hash_code() == int_type_id) { s << cast<int>(val); }
	else if (val.type().hash_code() == uint_type_id) { s << cast<uint>(val); }
	else if (val.type().hash_code() == ssize_type_id) { s << cast<ssize_t>(val); }
	else if (val.type().hash_code() == size_type_id) { s << cast<size_t>(val); }
	else if (val.type().hash_code() == float_type_id) { s << cast<float>(val); }
	else if (val.type().hash_code() == double_type_id) { s << cast<double>(val); }
	else if (val.type().hash_code() == jsonval_type_id) { s << Json::FastWriter{}.write(cast<Json::Value>(val)); }
	else { s << "null"; }
	return s;
}

void serialize_v(Json::Value& s, const std::any& val) {
	if (val.type().hash_code() == string_type_id) { s.append(cast<std::string>(val)); }
//	else if (val.type().hash_code() == map_type_id) { serialize_c(s, cast<map_t>(val)); }
	else if (val.type().hash_code() == umap_type_id) { serialize_c(s, cast<cjson::object_t>(val)); }
	else if (val.type().hash_code() == array_type_id) { serialize_c(s, cast<cjson::array_t>(val)); }
	else if (val.type().hash_code() == stringv_type_id) { s.append(std::string{ cast<std::string_view>(val) }); }
	else if (val.type().hash_code() == cchars_type_id) { s.append(cast<const char*>(val)); }
	else if (val.type().hash_code() == nullptr_type_id) { s.append(Json::nullValue); }
	else if (val.type().hash_code() == bool_type_id) { s.append(cast<bool>(val)); }
	else if (val.type().hash_code() == int_type_id) { s.append(cast<int>(val)); }
	else if (val.type().hash_code() == uint_type_id) { s.append(cast<uint>(val)); }
	else if (val.type().hash_code() == ssize_type_id) { s.append((Json::Value::Int64)cast<ssize_t>(val)); }
	else if (val.type().hash_code() == size_type_id) { s.append((Json::Value::UInt64)cast<size_t>(val)); }
	else if (val.type().hash_code() == float_type_id) { s.append(cast<float>(val)); }
	else if (val.type().hash_code() == double_type_id) { s.append(cast<double>(val)); }
	else { s.append(Json::nullValue); }
}

std::string cjson::serialize(object_t&& document) {
	std::stringstream ss;
	ss << std::boolalpha;
	serialize_v(ss, document);
	return ss.str();
}