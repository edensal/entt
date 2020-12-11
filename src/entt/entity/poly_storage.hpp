#ifndef ENTT_ENTITY_POLY_STORAGE_HPP
#define ENTT_ENTITY_POLY_STORAGE_HPP


#include <tuple>
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../core/utility.hpp"
#include "../poly/poly.hpp"
#include "fwd.hpp"


namespace entt {


template<typename Entity>
struct Storage: type_list<
    type_info() const,
    void(basic_registry<Entity> &, const Entity *, const std::size_t)
> {
    template<typename Base>
    struct type: Base {
        using entity_type = Entity;
        using size_type = std::size_t;

        type_info value_type() const {
            return poly_call<0>(*this);
        }

        void remove(basic_registry<entity_type> &owner, const entity_type *entity, const size_type length) {
            poly_call<1>(*this, owner, entity, length);
        }
    };

    template<typename Type>
    struct members {
        static type_info value_type() {
            return type_id<typename Type::value_type>();
        }

        static void remove(Type &self, basic_registry<Entity> &owner, const Entity *entity, const std::size_t length) {
            self.remove(owner, entity, entity + length);
        }
    };

    template<typename Type>
    using impl = value_list<
        &members<Type>::value_type,
        &members<Type>::remove
    >;
};


template<typename Entity, typename = void>
struct poly_storage_traits {
    using storage_type = poly<Storage<Entity>>;
};


}


#endif
