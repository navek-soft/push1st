#pragma once
#include "chttp.h"
#include <set>
#include <functional>
#include <memory>
#include "../inet/csocket.h"

namespace http {
	using routefn_t = std::function<void(const std::vector<std::string_view>&, const inet::csocket&, const std::string_view&, const http::uri_t&, const http::headers_t&, const std::string_view&)>;
	class crouter
	{
		struct key_t {
			std::string route;
			std::vector<std::string_view> path;
			routefn_t callback;
			key_t() { ; }
			explicit key_t(const std::string& r, routefn_t&& fn) : route{ r }, path{ std::move(explode(route)) }, callback{ fn }{; }
			explicit key_t(const std::string_view& r) : path{ std::move(explode(r)) } { ; }

			inline bool compare(const std::vector<std::string_view>& r, std::vector<std::string_view>& args) const {
				for (size_t n{ 1 }; n < path.size(); ++n) {
					if (path[n] == "?") { args.emplace_back(n < r.size() ? r[n] : std::string_view{} ); continue; }
					if (path[n] == "*") { args.insert(args.end(), r.begin() + n, r.end()); return true; }
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
				return l.path[1].compare(r.path[1]) == -1;
			}
			return false;
		}
	public:
		template<typename FN>
		inline void add(const std::string& route, FN&& callback) {
			key_t key{ route, callback };
			routes.emplace(std::string{ key.path.at(1) }, std::move(key));
		}
		inline bool call(const inet::csocket& fd, const std::string_view& method, const http::uri_t& uri, const http::headers_t& headers, const std::string_view& content) {
			if (uri.uriPathList.size() > 1) {
				std::vector<std::string_view> args;
				for (auto&& [it, end] { routes.equal_range(std::string{ uri.uriPathList[1] }) }; it != end; ++it) {
					if (args.clear(); it->second.compare(uri.uriPathList, args)) {
						it->second.callback(args, fd, method, uri, headers, content);
						return true;
					}
				}
			}
			return false;

		}
	private:
		std::multimap<std::string, key_t> routes;
	};

}

