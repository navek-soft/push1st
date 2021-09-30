#include <string>
#include <list>
#include <functional>
#include <memory>

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
		call(list);
	}
	bool call(const std::vector<std::string_view>& route);
private:
	node_t routeTree{};
};

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
