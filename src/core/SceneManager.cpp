#include "core/SceneManager.hpp"
#include "platform/IPlatform.hpp"
#include "core/InputManager.hpp"
#include "utils/Logger.hpp"

void SceneManager::push(std::unique_ptr<IScene> scene) {
    m_pending.emplace_back(PushCmd{std::move(scene)});
}

void SceneManager::replace(std::unique_ptr<IScene> scene) {
    m_pending.emplace_back(ReplaceCmd{std::move(scene)});
}

void SceneManager::pop() {
    m_pending.emplace_back(PopCmd{});
}

void SceneManager::handleInput(const InputManager& input) {
    if (!m_stack.empty()) {
        m_stack.back()->handleInput(input);
    }
}

void SceneManager::update(float dt) {
    if (!m_stack.empty()) {
        m_stack.back()->update(dt);
    }
}

void SceneManager::render(IPlatform& platform) {
    if (m_stack.empty()) return;

    // Find the lowest scene that needs rendering.
    // Walk down from the top: if a scene is NOT transparent, stop there.
    size_t renderFrom = m_stack.size() - 1;
    while (renderFrom > 0 && m_stack[renderFrom]->isTransparent()) {
        --renderFrom;
    }

    // Render bottom-up from that point
    for (size_t i = renderFrom; i < m_stack.size(); ++i) {
        m_stack[i]->render(platform);
    }
}

void SceneManager::applyPendingCommands() {
    // Swap to a local copy: onEnter()/onExit() may push new commands to
    // m_pending, which would invalidate iterators if we iterated directly.
    // Loop until no new commands are generated.
    while (!m_pending.empty()) {
        std::vector<Command> batch;
        std::swap(batch, m_pending);

        for (auto& cmd : batch) {
            std::visit([this](auto& c) {
                using T = std::decay_t<decltype(c)>;

                if constexpr (std::is_same_v<T, PushCmd>) {
                    LOG_DEBUG("SceneManager: push '" << c.scene->name() << "'");
                    c.scene->onEnter();
                    m_stack.push_back(std::move(c.scene));
                }
                else if constexpr (std::is_same_v<T, PopCmd>) {
                    if (!m_stack.empty()) {
                        LOG_DEBUG("SceneManager: pop '" << m_stack.back()->name() << "'");
                        m_stack.back()->onExit();
                        m_stack.pop_back();
                        if (!m_stack.empty()) {
                            m_stack.back()->onEnter();
                        }
                    } else {
                        LOG_WARN("SceneManager: pop on empty stack");
                    }
                }
                else if constexpr (std::is_same_v<T, ReplaceCmd>) {
                    if (!m_stack.empty()) {
                        LOG_DEBUG("SceneManager: replace '" << m_stack.back()->name()
                                  << "' with '" << c.scene->name() << "'");
                        m_stack.back()->onExit();
                        m_stack.pop_back();
                    }
                    c.scene->onEnter();
                    m_stack.push_back(std::move(c.scene));
                }
            }, cmd);
        }
    }
}

bool SceneManager::isEmpty() const {
    return m_stack.empty();
}

IScene* SceneManager::current() const {
    if (m_stack.empty()) return nullptr;
    return m_stack.back().get();
}

size_t SceneManager::size() const {
    return m_stack.size();
}
