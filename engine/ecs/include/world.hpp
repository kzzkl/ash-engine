#pragma once

#include "archetype.hpp"
#include "assert.hpp"
#include "entity.hpp"
#include "view.hpp"
#include <atomic>
#include <queue>

namespace ash::ecs
{
class mask_archetype;
struct entity_record
{
    mask_archetype* archetype;
    std::size_t index;

    std::uint32_t version;
};

using entity_record_map = std::deque<entity_record>;

class mask_archetype : public archetype
{
public:
    mask_archetype(
        const archetype_layout& layout,
        const std::unordered_map<component_id, component_index>& index_map)
        : archetype(layout)
    {
        for (const auto& [id, info] : layout)
            m_mask.set(index_map.at(id), true);
    }

    void add(entity_record* record)
    {
        archetype::add();

        record->archetype = this;
        record->index = m_record.size();
        ++record->version;

        m_record.push_back(record);
    }

    void move(std::size_t index, mask_archetype& target)
    {
        archetype::move(index, target);

        entity_record* target_record = m_record[index];
        target_record->archetype = &target;
        target_record->index = target.m_record.size();
        ++target_record->version;
        target.m_record.push_back(target_record);

        if (target_record != m_record.back())
        {
            m_record[index] = m_record.back();
            m_record[index]->index = index;
            ++m_record[index]->version;
        }
        m_record.pop_back();
    }

    const component_mask& mask() const noexcept { return m_mask; }

private:
    component_mask m_mask;
    std::vector<entity_record*> m_record;
};

template <typename Component>
class component_handle
{
public:
    using component_type = Component;
    using entity_type = entity;

public:
    component_handle() : m_pointer(nullptr), m_entity{INVALID_ENTITY_ID, 0}, m_record(nullptr) {}
    component_handle(entity entity, entity_record_map* record)
        : m_pointer(nullptr),
          m_entity(entity),
          m_record(record)
    {
        update();
    }

    component_type* get() const
    {
        if (m_record->at(m_entity.id).version != m_entity.version)
            update();
        return m_pointer;
    }

    entity_type entity() const noexcept { return m_entity; }

    component_type& operator*() const { return *get(); }
    component_type* operator->() const { return get(); }

private:
    void update() const
    {
        auto& record = m_record->at(m_entity.id);
        m_entity.version = record.version;

        auto handle = record.archetype->begin<component_type>() + record.index;
        m_pointer = &handle.component<component_type>();
    }

    mutable component_type* m_pointer;

    mutable entity_type m_entity;
    entity_record_map* m_record;
};

template <typename Component>
class read_handle : public component_handle<Component>
{
public:
    using component_type = Component;
    using entity_type = entity;
    using base_type = component_handle<Component>;

public:
    read_handle() : base_type() {}
    read_handle(entity entity, entity_record_map* record) : base_type(entity, record) {}

    const component_type* get() const
    {
        component_type* result = base_type::get();

        static constexpr bool has_mark = requires(Component t) { t.mark_read(); };
        if constexpr (has_mark) result->mark_read();

        return result;
    }

    const component_type& operator*() const { return *get(); }
    const component_type* operator->() const { return get(); }
};
template <typename Component>
using read = read_handle<Component>;

template <typename Component>
class write_handle : public read_handle<Component>
{
public:
    using component_type = Component;
    using entity_type = entity;
    using base_type = read_handle<Component>;

public:
    write_handle() : base_type() {}
    write_handle(entity entity, entity_record_map* record) : base_type(entity, record) {}

    component_type* get() const
    {
        component_type* result = const_cast<component_type*>(base_type::get());

        static constexpr bool has_mark = requires(Component t) { t.mark_write(); };
        if constexpr (has_mark) result->mark_write();

        return result;
    }

    component_type& operator*() const { return *get(); }
    component_type* operator->() const { return get(); }
};
template <typename Component>
using write = write_handle<Component>;

class world
{
public:
public:
    world() { register_component<all_entity>(); }
    ~world()
    {
        for (auto& [_, archetype] : m_archetypes)
        {
            archetype->clear();
        }
    }

    template <typename Component, typename Constructer, typename... Args>
    void register_component(Args&&... args)
    {
        register_component<Component>(new Constructer(std::forward<Args>(args)...));
    }

    template <typename Component>
    void register_component()
    {
        register_component<Component>(new default_component_constructer<Component>());
    }

    entity create()
    {
        if (m_free_entity.empty())
        {
            entity result = {static_cast<std::uint32_t>(m_entity_record.size()), 0};
            m_entity_record.push_back({nullptr, 0, 0});
            return result;
        }
        else
        {
            entity result = m_free_entity.front();
            m_free_entity.pop();

            return result;
        }
    }

    void release(entity e)
    {
        ++e.version;
        m_free_entity.push(e);

        m_entity_record[e.id].archetype = nullptr;
        m_entity_record[e.id].index = 0;
        ++m_entity_record[e.id].version;
    }

    template <typename... Components>
    void add(entity e)
    {
        auto& record = m_entity_record[e.id];
        if (record.archetype == nullptr)
        {
            mask_archetype* archetype = get_or_create_archetype<all_entity, Components...>();
            archetype->add(&record);
        }
        else
        {
            component_mask new_mask = record.archetype->mask() | make_mask<Components...>();
            if (new_mask == record.archetype->mask())
                return;

            auto iter = m_archetypes.find(new_mask);
            if (iter == m_archetypes.cend())
            {
                archetype_layout layout = record.archetype->layout();
                layout.insert(make_component_set<Components...>());

                mask_archetype* target = make_archetype(layout);
                record.archetype->move(record.index, *target);
            }
            else
            {
                record.archetype->move(record.index, *iter->second);
            }
        }
    }

    template <typename... Components>
    void remove(entity e)
    {
        auto& record = m_entity_record[e.id];

        component_mask new_mask = record.archetype->mask() ^ make_mask<Components...>();
        if (new_mask == record.archetype->mask())
            return;

        auto iter = m_archetypes.find(new_mask);
        if (iter == m_archetypes.cend())
        {
            archetype_layout layout = record.archetype->layout();
            layout.erase(make_component_set<Components...>());

            mask_archetype* target = make_archetype(layout);
            record.archetype->move(record.index, *target);
        }
        else
        {
            record.archetype->move(record.index, *iter->second);
        }
    }

    template <typename Component, template <typename T> class Handle = write>
    Handle<Component> component(entity e)
    {
        ASH_ASSERT(has_component<Component>(e));
        return Handle<Component>(e, &m_entity_record);
    }

    template <typename Component>
    bool has_component(entity e)
    {
        auto archetype = m_entity_record[e.id].archetype;
        auto component_index = m_component_index[component_id_v<Component>];

        return archetype->mask().test(component_index);
    }

    template <typename... Components>
    view<Components...>* make_view()
    {
        component_mask m = make_mask<Components...>();
        auto v = std::make_unique<view<Components...>>(m);

        for (auto& [mask, archetype] : m_archetypes)
        {
            if ((m & mask) == m)
                v->add_archetype(archetype.get());
        }

        auto result = v.get();
        m_views.push_back(std::move(v));
        return result;
    }

    bool vaild(entity e) const noexcept { return m_entity_record[e.id].version == e.version; }

private:
    template <typename T>
    struct index_generator
    {
    public:
        using index_type = T;

    public:
        index_generator(index_type base = 0) : m_next(base) {}
        index_type new_index() { return m_next.fetch_add(1); }

    private:
        std::atomic<index_type> m_next;
    };

    template <typename Component>
    void register_component(component_constructer* constructer)
    {
        auto iter = m_component_index.find(component_id_v<Component>);
        if (iter != m_component_index.end())
            return;

        component_index index = m_component_index_generator.new_index();
        m_component_index[component_id_v<Component>] = index;

        auto info =
            std::make_unique<component_info>(sizeof(Component), alignof(Component), constructer);
        if (m_component_info.size() <= index)
            m_component_info.resize(index + 1);
        m_component_info[index] = std::move(info);
    }

    template <typename... Components>
    component_set make_component_set()
    {
        component_set result;
        component_list<Components...>::each([&result, this]<typename T>() {
            component_index index = m_component_index[component_id_v<T>];
            result.push_back(std::make_pair(component_id_v<T>, m_component_info[index].get()));
        });
        return result;
    }

    template <typename... Components>
    component_mask make_mask()
    {
        component_mask result;
        (result.set(m_component_index[component_id_v<Components>]), ...);
        return result;
    }

    template <typename... Components>
    mask_archetype* get_or_create_archetype()
    {
        component_mask mask = make_mask<Components...>();
        auto& result = m_archetypes[mask];

        if (result == nullptr)
            return make_archetype<Components...>();
        else
            return result.get();
    }

    template <typename... Components>
    mask_archetype* make_archetype()
    {
        archetype_layout layout;
        layout.insert(make_component_set<Components...>());
        return make_archetype(layout);
    }

    mask_archetype* make_archetype(const archetype_layout& layout)
    {
        auto result = std::make_unique<mask_archetype>(layout, m_component_index);

        for (auto& v : m_views)
        {
            if ((v->mask() & result->mask()) == v->mask())
                v->add_archetype(result.get());
        }

        return (m_archetypes[result->mask()] = std::move(result)).get();
    }

    entity_record_map m_entity_record;
    std::queue<entity> m_free_entity;

    std::unordered_map<component_mask, std::unique_ptr<mask_archetype>> m_archetypes;

    std::unordered_map<component_id, component_index> m_component_index;
    std::vector<std::unique_ptr<component_info>> m_component_info;

    std::vector<std::unique_ptr<view_base>> m_views;

    index_generator<component_index> m_component_index_generator;
};
} // namespace ash::ecs