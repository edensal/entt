#include <functional>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>
#include <entt/poly/poly.hpp>

struct Defined: entt::type_list<
    void(),
    void(int),
    int() const,
    void(),
    int(int)
> {
    template<typename Base>
    struct type: Base {
        void incr() { entt::poly_call<0>(*this); }
        void set(int v) { entt::poly_call<1>(*this, v); }
        int get() const { return entt::poly_call<2>(*this); }
        void decr() { entt::poly_call<3>(*this); }
        int mul(int v) { return entt::poly_call<4>(*this, v); }
    };
};

template<typename Type>
inline constexpr auto entt::poly_impl<Defined, Type> =
    std::make_tuple(
        &Type::incr,
        &Type::set,
        &Type::get,
        +[](Type &self) { self.decrement(); },
        +[](Type &self, double v) -> double { return self.multiply(v); }
    );

struct impl {
    void incr() { ++value; }
    void set(int v) { value = v; }
    int get() const { return value; }
    void decrement() { --value; }
    double multiply(double v) { return v * value; }
    int value{};
};

TEST(PolyDefined, Functionalities) {
    impl instance{};

    entt::poly<Defined> empty{};
    entt::poly<Defined> in_place{std::in_place_type<impl>, 3};
    entt::poly<Defined> alias{std::ref(instance)};
    entt::poly<Defined> value{impl{}};

    ASSERT_FALSE(empty);
    ASSERT_TRUE(in_place);
    ASSERT_TRUE(alias);
    ASSERT_TRUE(value);

    ASSERT_EQ(empty.type(), entt::type_info{});
    ASSERT_EQ(in_place.type(), entt::type_id<impl>());
    ASSERT_EQ(alias.type(), entt::type_id<impl>());
    ASSERT_EQ(value.type(), entt::type_id<impl>());

    ASSERT_EQ(alias.data(), &instance);
    ASSERT_EQ(std::as_const(alias).data(), &instance);

    empty = impl{};

    ASSERT_TRUE(empty);
    ASSERT_NE(empty.data(), nullptr);
    ASSERT_NE(std::as_const(empty).data(), nullptr);
    ASSERT_EQ(empty.type(), entt::type_id<impl>());
    ASSERT_EQ(empty.get(), 0);

    empty.template emplace<impl>(3);

    ASSERT_TRUE(empty);
    ASSERT_EQ(empty.get(), 3);

    entt::poly<Defined> ref = in_place.ref();

    ASSERT_TRUE(ref);
    ASSERT_NE(ref.data(), nullptr);
    ASSERT_EQ(ref.data(), in_place.data());
    ASSERT_EQ(std::as_const(ref).data(), std::as_const(in_place).data());
    ASSERT_EQ(ref.type(), entt::type_id<impl>());
    ASSERT_EQ(ref.get(), 3);

    entt::poly<Defined> null{};
    std::swap(empty, null);

    ASSERT_FALSE(empty);

    entt::poly<Defined> copy = in_place;

    ASSERT_TRUE(copy);
    ASSERT_EQ(copy.get(), 3);

    entt::poly<Defined> move = std::move(copy);

    ASSERT_TRUE(move);
    ASSERT_FALSE(copy);
    ASSERT_EQ(move.get(), 3);
}

TEST(PolyDefined, Owned) {
    entt::poly<Defined> poly{impl{}};
    auto *ptr = static_cast<impl *>(poly.data());

    ASSERT_TRUE(poly);
    ASSERT_NE(poly.data(), nullptr);
    ASSERT_NE(std::as_const(poly).data(), nullptr);
    ASSERT_EQ(ptr->value, 0);
    ASSERT_EQ(poly.get(), 0);

    poly.set(1);
    poly.incr();

    ASSERT_EQ(ptr->value, 2);
    ASSERT_EQ(poly.get(), 2);
    ASSERT_EQ(poly.mul(3), 6);

    poly.decr();

    ASSERT_EQ(ptr->value, 1);
    ASSERT_EQ(poly.get(), 1);
    ASSERT_EQ(poly.mul(3), 3);
}

TEST(PolyDefined, Alias) {
    impl instance{};
    entt::poly<Defined> poly{std::ref(instance)};

    ASSERT_TRUE(poly);
    ASSERT_NE(poly.data(), nullptr);
    ASSERT_NE(std::as_const(poly).data(), nullptr);
    ASSERT_EQ(instance.value, 0);
    ASSERT_EQ(poly.get(), 0);

    poly.set(1);
    poly.incr();

    ASSERT_EQ(instance.value, 2);
    ASSERT_EQ(poly.get(), 2);
    ASSERT_EQ(poly.mul(3), 6);

    poly.decr();

    ASSERT_EQ(instance.value, 1);
    ASSERT_EQ(poly.get(), 1);
    ASSERT_EQ(poly.mul(3), 3);
}
