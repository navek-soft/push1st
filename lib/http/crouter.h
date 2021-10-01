#pragma once
#include "chttp.h"
#include <list>
#include <functional>
#include <memory>
#include "../inet/csocket.h"

namespace http {
	using routefn_t = std::function<void(const std::vector<std::string_view>&, const inet::csocket&, const std::string_view&, const http::uri_t&, const http::headers_t&, std::string&&)>;
	class crouter
	{
		struct node_t;
		using list_t = std::list<std::pair<std::string, std::shared_ptr<node_t>>>;
		struct node_t {
			std::shared_ptr<node_t> nextBucket;
			list_t List;
			routefn_t Fn;
			std::shared_ptr<node_t> next() { return nextBucket ? nextBucket : (nextBucket = std::make_shared<node_t>()); }
		};
		inline list_t::iterator emplace(node_t* node, std::string_view part);
		void build(std::string_view path, routefn_t&& fn);
	public:
		template<typename FN>
		inline void add(const std::string& route, FN&& callback) {
			std::string_view path{ route };
			if (path.back() == '/') { path.remove_suffix(1); }
			build(path, std::move(callback));
		}
		bool call(const inet::csocket& fd, const std::string_view& method, const http::uri_t& uri, const http::headers_t& headers, std::string&& content);
	private:
		node_t routeTree{};
	};

}

