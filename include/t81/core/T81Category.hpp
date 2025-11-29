//======================================================================
// T81Category.hpp – Category theory in balanced ternary
//                  The final mathematical abstraction in the T81 universe
//======================================================================
#pragma once

#include "t81/core/T81Symbol.hpp"
#include "t81/T81Tree.hpp"
#include "t81/T81Map.hpp"
#include "t81/T81Set.hpp"
#include "t81/T81Polynomial.hpp"
#include <functional>
#include <optional>

namespace t81 {

// Forward declarations
template <typename Obj, typename Mor> class T81Category;
template <typename C, typename D> class T81Functor;

// ======================================================================
// T81Morphism – A morphism with source, target, and composition law
// ======================================================================
template <typename Obj, typename Mor>
struct T81Morphism {
    T81Symbol name;           // symbolic identity of the morphism
    Obj       source;
    Obj       target;
    Mor       data;           // underlying computational object

    constexpr T81Morphism(T81Symbol n, Obj s, Obj t, Mor d)
        : name(n), source(s), target(t), data(std::move(d)) {}

    [[nodiscard]] constexpr bool composable_with(const T81Morphism& other) const noexcept {
        return target == other.source;
    }
};

// ======================================================================
// T81Category – A category is a set of objects and morphisms
// ======================================================================
template <typename Obj, typename Mor>
class T81Category {
    T81Set<Obj> objects_;
    T81Map<T81Symbol, T81Morphism<Obj,Mor>> morphisms_;
    T81Map<Obj, T81Symbol> identity_map_;  // identity morphism per object

public:
    using object_type   = Obj;
    using morphism_type = Mor;

    //===================================================================
    // Construction
    //===================================================================
    constexpr T81Category() = default;

    // Add object
    constexpr T81Category& add_object(Obj obj, T81Symbol identity_name = {}) {
        objects_ = objects_.insert(obj);
        if (identity_name) {
            auto id_mor = T81Morphism<Obj,Mor>(identity_name, obj, obj, Mor{});
            morphisms_[identity_name] = id_mor;
            identity_map_[obj] = identity_name;
        }
        return *this;
    }

    // Add morphism
    constexpr T81Category& add_morphism(T81Symbol name, Obj src, Obj dst, Mor data) {
        if (!objects_.contains(src) || !objects_.contains(dst))
            return *this; // invalid
        morphisms_[name] = T81Morphism<Obj,Mor>(name, src, dst, std::move(data));
        return *this;
    }

    //===================================================================
    // Composition – the heart of category theory
    //===================================================================
    [[nodiscard]] std::optional<T81Morphism<Obj,Mor>> compose(
        const T81Symbol& f_name,
        const T81Symbol& g_name
    ) const {
        auto f_it = morphisms_.find(f_name);
        auto g_it = morphisms_.find(g_name);
        if (!f_it || !g_it) return std::nullopt;

        const auto& f = *f_it;
        const auto& g = *g_it;
        if (!f.composable_with(g)) return std::nullopt;

        // Compute composite data — depends on Mor type
        Mor composite_data = compose_data(f.data, g.data);
        T81Symbol composite_name = T81Symbol::intern(f.name.str() + " ∘ " + g.name.str());

        return T81Morphism<Obj,Mor>(composite_name, g.source, f.target, std::move(composite_data));
    }

    //===================================================================
    // Identity
    //===================================================================
    [[nodiscard]] constexpr T81Symbol identity_of(Obj obj) const {
        return identity_map_.contains(obj) ? identity_map_.at(obj) : symbols::NO_IDENTITY;
    }

    //===================================================================
    // Functor support
    //===================================================================
    template <typename C2, typename D2>
    friend class T81Functor;

private:
    // Default composition — override via specialization
    static Mor compose_data(const Mor& f, const Mor& g) {
        // Default: assume Mor supports composition (e.g., function, matrix)
        if constexpr (requires { f(g); }) {
            return f(g);
        } else {
            static_assert(std::is_same_v<Mor, void>, "Specialize compose_data for this morphism type");
            return Mor{};
        }
    }
};

// ======================================================================
// T81Functor – Maps between categories
// ======================================================================
template <typename C, typename D>
class T81Functor {
    const C& source_;
    const D& target_;
    T81Map<typename C::object_type, typename D::object_type>   object_map_;
    T81Map<T81Symbol, T81Symbol>                               morphism_map_;

public:
    constexpr T81Functor(const C& src, const D& dst)
        : source_(src), target_(dst) {}

    constexpr T81Functor& map_object(typename C::object_type src, typename D::object_type dst) {
        object_map_[src] = dst;
        return *this;
    }

    constexpr T81Functor& map_morphism(T81Symbol src_mor, T81Symbol dst_mor) {
        morphism_map_[src_mor] = dst_mor;
        return *this;
    }

    [[nodiscard]] constexpr auto apply_object(typename C::object_type obj) const {
        return object_map_.contains(obj) ? object_map_.at(obj) : typename D::object_type{};
    }
};

// ======================================================================
// Predefined categories – the foundations of mathematics
// ======================================================================
namespace categories {

    // Category of T81Vector spaces
    using Vec = T81Category<T81Vector<3>, T81Matrix<T81Float<72,9>,3,3>>;

    // Category of T81Set (discrete category)
    using Set = T81Category<T81Set<T81Symbol>, std::function<T81Set<T81Symbol>(T81Set<T81Symbol>)>>;

    // Category of types and functions
    using Type = T81Category<T81Symbol, std::function<void*(void*)>>;

} // namespace categories

// ======================================================================
// Example: This is how the future does mathematics
// ======================================================================
/*
auto C = categories::Vec()
    .add_object(Vec3::zero(), symbols::ORIGIN)
    .add_morphism(symbols::ROT90, Vec3::zero(), Vec3::zero(), rotation_matrix_90);

auto f = symbols::TRANSLATE;
auto g = symbols::ROT90;
auto h = C.compose(f, g);  // exact composition

static_assert(h.has_value());
*/
