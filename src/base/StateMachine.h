#pragma once

namespace sr::base {

template<typename owner_t, typename state_id_t>
class State {
public:
    virtual ~State() = default;

    virtual state_id_t GetID() const = 0;
    virtual void OnEnter(owner_t&) { }
    virtual void OnExit(owner_t&) { }
    virtual void OnExcute(owner_t&) = 0;
};

template<typename owner_t, typename state_id_t>
class StateMachine {
public:
    using state_t = State<owner_t, state_id_t>;

    bool HasState() const
    {
        return m_CurrentState != nullptr;
    }

    const state_t* CurrentState() const
    {
        return m_CurrentState;
    }

    StateMachine() = default;

    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;
    StateMachine(StateMachine&&) = default;
    StateMachine& operator=(StateMachine&&) = default;

protected:
    void SetInitialState(owner_t& owner, state_t& state)
    {
        m_CurrentState = &state;
        m_CurrentState->OnEnter(owner);
    }

    void ChangeState(owner_t& owner, state_t& state)
    {
        if (m_CurrentState == &state) {
            return;
        }

        if (m_CurrentState != nullptr) {
            m_CurrentState->OnExit(owner);
        }

        m_CurrentState = &state;
        m_CurrentState->OnEnter(owner);
    }

    void ClearState(owner_t& owner)
    {
        if (m_CurrentState != nullptr) {
            m_CurrentState->OnExit(owner);
            m_CurrentState = nullptr;
        }
    }

    void UpdateCurrentState(owner_t& owner)
    {
        if (m_CurrentState != nullptr) {
            m_CurrentState->OnExcute(owner);
        }
    }

private:
    state_t* m_CurrentState = nullptr;
};

} // namespace sr::base

