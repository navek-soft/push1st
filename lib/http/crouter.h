#pragma once
#include "chttp.h"
#include <list>
#include <functional>
#include <memory>

namespace http {
	class crouter
	{
		struct node_t;
		using list_t = std::list<std::pair<std::string, std::shared_ptr<node_t>>>;
		struct node_t {
			std::shared_ptr<node_t> nextBucket;
			list_t List;
			std::function<void(const std::vector<std::string_view>&)> Fn;
			std::shared_ptr<node_t> next() { return nextBucket ? nextBucket : (nextBucket = std::make_shared<node_t>()); }
		};
		inline list_t::iterator emplace(node_t* node, std::string_view part);
		void build(std::string_view path, std::function<void(const std::vector<std::string_view>&)>&& fn);
	public:
		template<typename FN>
		inline void add(const std::string& route, FN&& callback) {
			std::string_view path{ route };
			if (path.front() == '/') { path.remove_prefix(1); }
			if (path.back() == '/') { path.remove_suffix(1); }
			build(path, std::move(callback));
		}
		inline bool call(std::string_view route) {
			std::string_view path{ route };
			std::vector<std::string_view> list;
			if (path.front() == '/') { path.remove_prefix(1); }
			if (path.back() == '/') { path.remove_suffix(1); }
			for (size_t npos = path.find_first_of('/', 0); ; path.remove_prefix(npos + 1), npos = path.find_first_of('/', 0)) {
				list.emplace_back(path.substr(0, npos));
				if (npos == std::string_view::npos) break;
			}
			return call(list);
		}
		bool call(const std::vector<std::string_view>& route);
	private:
		node_t routeTree{};
	};

}

