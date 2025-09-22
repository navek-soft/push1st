#pragma once

#include <memory>
#include <utility>

template <class T>
class csharedfactory {
   public:
    template <typename C = T, typename... Args>
    static std::shared_ptr<C> MakeShared(Args&&... args) {
        static_assert(std::is_base_of_v<T, C>, "C must be derived from T");
        return std::shared_ptr<C>(new C {std::forward<Args>(args)...});
    }
};

template <class T>
class cuniquefactory {
   public:
    template <typename C = T, typename... Args>
    static std::unique_ptr<C> MakeUnique(Args&&... args) {
        static_assert(std::is_base_of_v<T, C>, "C must be derived from T");
        return std::unique_ptr<C>(new C {std::forward<Args>(args)...});
    }
};
