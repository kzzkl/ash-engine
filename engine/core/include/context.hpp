#pragma once

#include "assert.hpp"
#include "core_exports.hpp"
#include "dictionary.hpp"
#include "log.hpp"
#include "task_manager.hpp"
#include "timer.hpp"
#include "uuid.hpp"
#include "world.hpp"
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ash::core
{
class context;
class CORE_API submodule
{
public:
    submodule(std::string_view name) noexcept;
    virtual ~submodule() = default;

    virtual bool initialize(const dictionary& config) = 0;
    virtual void shutdown(){};

    inline std::string_view name() const noexcept { return m_name; }

protected:
    template <typename T>
    T& module();

private:
    friend class context;

    std::string m_name;
    context* m_context;
};

template <typename T>
concept derived_from_submodule = std::is_base_of<submodule, T>::value;

template <derived_from_submodule T>
struct submodule_trait
{
    static constexpr uuid id = T::id;
};

class CORE_API context
{
public:
    using module_list = std::unordered_map<uuid, std::unique_ptr<submodule>, uuid_hash>;

public:
    context(std::string_view config_path);

    template <typename T>
    T& module()
    {
        return *static_cast<T*>(m_modules[submodule_trait<T>::id].get());
    }

    template <>
    ash::task::task_manager& module<ash::task::task_manager>()
    {
        return *m_task;
    }

    template <>
    ash::ecs::world& module<ash::ecs::world>()
    {
        return *m_world;
    }

    template <>
    ash::core::timer& module<ash::core::timer>()
    {
        return *m_timer;
    }

protected:
    template <derived_from_submodule T, typename... Args>
    void install_submodule(Args&&... args)
    {
        uuid id = submodule_trait<T>::id;
        if (m_modules[id] == nullptr)
        {
            auto m = std::make_unique<T>(std::forward<Args>(args)...);
            m->m_context = this;
            m->initialize(m_config[m->name().data()]);
            log::info("Module installed successfully: {}.", m->name());
            m_modules[id] = std::move(m);
        }
        else
        {
            log::warn("The module is already installed.");
        }
    }

    void shutdown_submodule();

private:
    void load_config(std::string_view config_path);

    std::map<std::string, dictionary> m_config;

    module_list m_modules;
    std::unique_ptr<ash::task::task_manager> m_task;
    std::unique_ptr<ash::ecs::world> m_world;

    std::unique_ptr<timer> m_timer;
};

template <typename T>
T& submodule::module()
{
    return m_context->module<T>();
}
} // namespace ash::core