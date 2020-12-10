# Crash Course: poly

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
  * [Other libraries](#other-libraries)
* [Concept and implementation](#concept-and-implementation)
  * [Deduced interface](#deduced-interface)
  * [Defined interface](#defined-interface)
  * [Fullfill a concept](#fullfill-a-concept)
* [Static polymorphism in the wild](#static-polymorphism-in-the-wild)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

Static polymorphism is a very powerful tool in C++, albeit sometimes cumbersome
to obtain.<br/>
This module aims to make it simple and easy to use.

The library allows to define _concepts_ as interfaces to fullfill with concrete
classes withouth having to inherit from a common base.<br/>
This is, among others, one of the advantages of static polymorphism in general
and of a generic wrapper like that offered by the `poly` class template in
particular.<br/>
What users get is an object that can be passed around as such and not through a
reference or a pointer, as happens when it comes to working with dynamic
polymorphism.

Since the `poly` class template makes use of `entt::any` internally, it also
supports most of its feature. Among the most important, the possibility to
create aliases to existing and thus unmanaged objects. This allows users to
exploit the static polymorphism while maintaining ownership of objects.<br/>
Likewise, the `poly` class template also benefits from the small buffer
optimization offered by the `entt::any` class and therefore minimizes the number
of allocations, avoiding them altogether where possible.

## Other libraries

There are some very interesting libraries regarding static polymorphism.<br/>
Among all, the two that I prefer are:

* [`dyno`](https://github.com/ldionne/dyno): runtime polymorphism done right.
* [`Poly`](https://github.com/facebook/folly/blob/master/folly/docs/Poly.md):
  a class template that makes it easy to define a type-erasing polymorphic
  object wrapper.

The former is admittedly an experimental library, with many interesting ideas.
I've some doubts about the usefulness of some feature in real world projects,
but perhaps my lack of experience comes into play here. In my opinion, its only
flaw is the API which I find slightly more cumbersome than other solutions.<br/>
The latter was undoubtedly a source of inspiration for this module, although I
opted for different choices in the implementation of both the final API and some
feature.

Either way, the authors are gurus of the C++ community, people I only have to
learn from.

# Concept and implementation

The first thing to do to create a _type-erasing polymorphic object wrapper_ (to
use the terminology introduced by Eric Niebler) is to define a _concept_ that
types will have to adhere to.<br/>
For this purpose, the library offers a single class that supports both deduced
and fully defined interfaces. Although having interfaces deduced automatically
is convenient and allows users to write less code in most cases, this has some
limitations and it's therefore useful to be able to get around the deduction by
providing a custom definition for the static virtual table.

Once the interface is defined, it will be sufficient to provide a generic
implementation to fullfill the concept.<br/>
Also in this case, the library allows customizations based on types or families
of types, so as to be able to go beyond the generic case where necessary.

## Deduced interface

This is how a concept with a deduced interface is introduced:

```cpp
struct Drawable: entt::type_list<> {
    template<typename Base>
    struct type: Base {
        void draw() { this->template invoke<0>(*this); }
    };
};
```

It's recognizable by the fact that it inherits from an empty type list.<br/>
Functions can also be const, accept any number of paramters and return a type
other than `void`:

```cpp
struct Drawable: entt::type_list<> {
    template<typename Base>
    struct type: Base {
        bool draw(int pt) const { return this->template invoke<0>(*this, pt); }
    };
};
```

In this case, all parameters must be passed to `invoke` after the reference to
`this` and the return value is whatever the internal call returns.<br/>
As for `invoke`, this is a name that is injected into the _concept_ through
`Base`, from which one must necessarily inherit. Since it's also a dependent
name, the `this-> template` form is unfortunately necessary due to the rules of
the language. However, there exists also an alternative that goes through an
external call:

```cpp
template<typename Base>
struct Drawable: Base {
    void draw() { entt::poly_call<0>(*this); }
};
```

Once the _concept_ is defined, users must specialize a template variable called
`poly_impl` to tell the system how any type can satisfy its requirements.<br/>
The index passed as a template parameter to either `invoke` or `poly_call`
refers to how this template variable is specialized.

## Defined interface

A fully defined concept is no different to one for which the interface is
deduced, with the only difference that the list of types is not empty this time:

```cpp
struct Drawable: entt::type_list<void()> {
    template<typename Base>
    struct type: Base {
        void draw() { entt::poly_call<0>(*this); }
    };
};
```

Again, parameters and return values other than `void` are allowed. Also, the
function type must be const when the method to bind to it is const:

```cpp
struct Drawable: entt::type_list<bool(int) const> {
    template<typename Base>
    struct type: Base {
        bool draw(int pt) const { return entt::poly_call<0>(*this, pt); }
    };
};
```

Why should a user fully define a concept if the function types are the same as
the deduced ones?<br>
Because, in fact, this is exactly the limitation that can be worked around by
manually defining the static virtual table.

When things are deduced, there is an implicit constraint.<br/>
If the concept exposes a member function called `draw` with function type
`void()`, a concept can be satisfied only if:

* Either by a class that exposes a member function with the same name and the
  same signature.

* Or through a lambda that makes use of existing member functions from the
  interface itself.

In other words, it's not possible to make use of functions not belonging to the
interface, even if they are present in the types that fulfill the concept.<br/>
Similarly, it's not possible to deduce a function in the static virtual table
with a function type different from that of the associated member function in
the interface itself.

Explicitly defining a static virtual table suppresses the deduction step and
allows maximum flexibility when specializing `poly_impl` instead.

## Fullfill a concept

The specialization of the `poly_impl` variable template that defines how a
concept is fulfilled looks like the following:

```cpp
template<typename Type>
inline constexpr auto entt::poly_impl<Drawable, Type> = std::make_tuple(&Type::draw);
```

In this case, it's stated that the `draw` method of a generic type will be
enough to satisfy the requirements of the `Drawable` concept.<br/>
The `poly_impl` variable template can be specialized in a generic way as in the
example above, or for a specific type where this satisfies the requirements
differently. Moreover, it's easy to specialize it for families of types:

```cpp
template<typename Type>
inline constexpr auto entt::poly_impl<Drawable, std::vector<Type>> = std::make_tuple(
    +[](const Type &self) {
        for(auto &&element: self) {
            element.draw();
        }
    }
);
```

Finally, as shown above, an implementation doesn't have to consist of just
member functions. Free functions and non-capturing decayed lambdas are valid
alternatives to fill any gaps in the interface of a type:

```cpp
template<typename Type>
void print(Type &self) { self.print(); }

template<typename Type>
inline constexpr auto entt::poly_impl<Drawable, Type> = entt::value_list<&print<Type>>{};
```

Likewise, as long as the parameter types and return type support conversions to
and from the function type referenced in the static virtual table, the actual
implementation may differ in its function type since it's erased internally.

Refer to the inline documentation for more details.

# Static polymorphism in the wild

Once the _concept_ and implementation have been introduced, it will be possible
to use the `poly` class template to contain instances that meet the
requirements:

```cpp
using drawable = entt::poly<Drawable>;

struct circle {
    void draw() { /* ... */ }
};

struct square {
    void draw() { /* ... */ }
};

// ...

drawable d{circle{}};
d->draw();

d = square{};
d->draw();
```

The `poly` class template offers a wide range of constructors, from the default
one (which will return an uninitialized `poly` object) to the copy and move
constructor, as well as the ability to create objects in-place.<br/>
Among others, there is a constructor that allows users to wrap unmanaged objects
in a `poly` instance:

```cpp
circle c;
drawable d{std::ref(c)};
```

In this case, although the interface of the `poly` object doesn't change, it
won't construct any element or take care of destroying the referenced object.

Note also how the underlying concept is accessed via a call to `operator->` and
not directly as `d.draw()`.<br/>
This allows users to decouple the API of the wrapper from that of the concept.
Therefore, where `d.data()` will invoke the `data` member function of the poly
object, `d->data()` will map directly to the functionality exposed by the
underlying concept.
