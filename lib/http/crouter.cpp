#include "crouter.h"

using namespace http;

bool crouter::call(const std::vector<std::string_view>& route) {
	std::vector<std::string_view> args;
	auto node{ &routeTree };
	list_t::iterator it;
	for (auto&& r_it{ route.begin() }; node and r_it != route.end(); ++r_it) {
		for (it = node->List.begin(); it != node->List.end(); ++it) {
			if (it->first == "?" or it->first == *r_it) {
				if (it->first == "?") { args.emplace_back(*r_it); }
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