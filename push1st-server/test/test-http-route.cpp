#include <string>
#include <set>
#include <functional>

class crouter
{
	struct key_t {
		std::string route;
		std::vector<std::string_view> path;
		std::function<void(const std::vector<std::string_view>&)> callback;
		key_t() { ; }
		explicit key_t(const std::string& r, std::function<void(const std::vector<std::string_view>&)>&& fn) : route{ r }, path{ std::move(explode(route)) }, callback{ fn }{; }
		explicit key_t(const std::string_view& r) : path{ std::move(explode(r)) } { ; }

		inline bool compare(std::vector<std::string_view>&& r, std::vector<std::string_view>& args) const {
			for (size_t n{ 2 }; n < path.size(); ++n) {
				if (path[n] == "?") { args.emplace_back(r[n]);  continue; }
				if (path[n] == "*") { 
					if (r.size() > n) { 
						args.insert(args.end(),r.begin() + n, r.end()); 
						return true; 
					} 
					return false; 
				}
				if (path[n] != r[n]) { return false; }
			}
			return true;
		}

		static inline std::vector<std::string_view> explode(std::string_view route) {
			std::vector<std::string_view> path;
			if (!route.empty()) {
				if (route.front() == '/') { path.emplace_back(route.data(), 1);  route.remove_prefix(1); }
				if (route.back() == '/') { route.remove_suffix(1); }
				for (size_t npos = route.find_first_of('/', 0); ; route.remove_prefix(npos + 1), npos = route.find_first_of('/', 0)) {
					path.emplace_back(route.substr(0, npos));
					if (npos != std::string_view::npos) {
						continue;
					}
					break;
				}
			}
			return path;
		}
	};
	static inline bool less(const key_t& l, const key_t& r) {
		if (l.path.size() > 1 and r.path.size() > 1) {
			return l.path[1].compare(r.path[1]) != 0;
		}
		return false;
	}
public:
	template<typename FN>
	inline void add(const std::string& route, FN&& callback) {
		routes.emplace(route, callback);
	}
	
	inline bool call(std::string_view route) {
		auto&& key{ key_t{ route } };
		std::vector<std::string_view> args;
		for (auto&& [it, end] { routes.equal_range(key) }; it != end; ++it) {
			if (args.clear(); it->compare(std::move(key.path), args)) {
				it->callback(args);
				return true;
			}
		}
		return false;
	}
	/*
	bool call(const std::vector<std::string_view>& route);
	*/
private:
	std::multiset<key_t, decltype(&less)> routes{ &less };
	//node_t routeTree{};
};
/*
bool crouter::call(const std::vector<std::string_view>& route) {
	std::vector<std::string_view> args;
	auto node{ &routeTree };
	list_t::iterator it;
	for (auto&& r_it{ route.begin() }; node and r_it != route.end(); ++r_it) {
		for (it = node->List.begin(); it != node->List.end(); ++it) {
			if (it->first == "?" or it->first == *r_it) {
				if ((r_it + 1) != route.end() or it->second->nextBucket) {
					node = it->second->nextBucket.get();
					break;
				}
				node->Fn(args);
				return true;
				
			}
			else if (it->first == "*" and (r_it + 1) != route.end()) {
				node->Fn(args);
				return true;
			}
		}
		if (it == node->List.end()) { break; }
	}
	return false;
}

crouter::list_t::iterator crouter::emplace(node_t* node, std::string_view part) {
	for (auto it{ node->List.begin() }; it != node->List.end(); ++it) {
		if (it->first != part) continue;
		return it;
	}
	if (part != "?" and part != "*") {
		node->List.emplace_front(part, std::make_shared<node_t>());
		return node->List.begin();
	}
	node->List.emplace_back(part, std::make_shared<node_t>());
	return --node->List.end();
}

void crouter::build(std::string_view path, std::function<void(const std::vector<std::string_view>&)>&& fn) {
	auto node{ &routeTree };
	for (size_t npos = path.find_first_of('/', 0); ; path.remove_prefix(npos + 1), npos = path.find_first_of('/', 0)) {
		auto&& it{ emplace(node, path.substr(0,npos)) };
		if (npos != std::string_view::npos) {
			node = it->second->next().get();
			continue;
		}
		node->Fn = fn;
		break;
	}
}
*/
int test_http_route(int argc, char* argv[]) {
	crouter route;
	route.add("/apps/?/?/events", [](const std::vector<std::string_view>& args) { 
		printf("/apps/?/?/events\n"); 
	});
	route.add("/api/?/channels/", [](const std::vector<std::string_view>& args) {
		printf("/api/?/channels/\n"); 
	});
	route.add("/api/?/users/*", [](const std::vector<std::string_view>& args) {
		printf("/api/?/users/*\n"); 
	});
	
	route.call("/api/test/test");
	route.call("/api/test1/users");
	route.call("/api/test1/users/wer/rrrr");
	route.call("/api/link/channels/");

	route.call("/apps/test1/users/events");
	return 0;
}
