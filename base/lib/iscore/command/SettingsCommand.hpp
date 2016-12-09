#pragma once
#include <boost/call_traits.hpp>
#include <iscore/command/Command.hpp>
namespace iscore
{
class ISCORE_LIB_BASE_EXPORT SettingsCommandBase
{
    public:
        virtual ~SettingsCommandBase();
        virtual void undo() const = 0;
        virtual void redo() const = 0;
};

template <typename T>
class SettingsCommand : public SettingsCommandBase
{
public:
  using parameter_t = T;
  using parameter_pass_t =
      typename boost::call_traits<typename T::param_type>::param_type;
  SettingsCommand() = default;
  SettingsCommand(typename T::model_type& obj, parameter_pass_t newval)
      : m_model{obj}, m_new{newval}
  {
    m_old = (m_model.*T::get())();
  }

  virtual ~SettingsCommand() = default;

  void undo() const override
  {
    (m_model.*T::set())(m_old);
  }

  void redo() const override
  {
    (m_model.*T::set())(m_new);
  }

  void update(typename T::model_type&, parameter_pass_t newval)
  {
    m_new = newval;
  }

private:
  typename T::model_type& m_model;
  typename T::param_type m_old, m_new;
};
}

#define ISCORE_SETTINGS_COMMAND_DECL(name)                     \
public:                                                        \
  using iscore::SettingsCommand<parameter_t>::SettingsCommand; \
  name() = default;                                            \
  static const CommandFactoryKey& static_key()                 \
  {                                                            \
    static const CommandFactoryKey var{#name};                 \
    return var;                                                \
  }                                                            \
                                                               \
private:
