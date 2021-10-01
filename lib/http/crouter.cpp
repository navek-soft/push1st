#include "crouter.h"

using namespace http;

bool crouter::call(const inet::csocket& fd, const std::string_view& method, const http::uri_t& uri, const http::headers_t& headers, std::string&& content) {
	std::vector<std::string_view> args;
	auto node{ &routeTree };
	list_t::iterator it;
	for (auto&& r_it{ uri.uriPathList.begin() }; node and r_it != uri.uriPathList.end(); ++r_it) {
		for (it = node->List.begin(); it != node->List.end(); ++it) {
			if (it->first == "?" or it->first == *r_it) {
				if (it->first == "?") { args.emplace_back(*r_it); }
				if ((r_it + 1) != uri.uriPathList.end() or it->second->nextBucket) {
					node = it->second->nextBucket.get();
					break;
				}
				node->Fn(args, fd, method, uri, headers, std::move(content));
				return true;

			}
			else if (it->first == "*" and (r_it + 1) != uri.uriPathList.end()) {
				node->Fn(args, fd, method, uri, headers, std::move(content));
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

void crouter::build(std::string_view path, routefn_t&& fn) {
	auto node{ &routeTree };
	if (path.front() == '/') {
		path.remove_prefix(1);
		auto&& it{ emplace(node, "/")};
		node = it->second->next().get();
	}
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