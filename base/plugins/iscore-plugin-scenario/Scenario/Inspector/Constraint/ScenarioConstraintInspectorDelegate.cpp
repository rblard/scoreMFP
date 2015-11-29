#include "ScenarioConstraintInspectorDelegate.hpp"
#include <Scenario/Commands/Scenario/Displacement/MoveEventMeta.hpp>


ScenarioConstraintInspectorDelegate::ScenarioConstraintInspectorDelegate(
        const ConstraintModel& cst):
    ConstraintInspectorDelegate{cst}
{
}

void ScenarioConstraintInspectorDelegate::updateElements()
{
}

void ScenarioConstraintInspectorDelegate::addWidgets_pre(
        std::list<QWidget*>& widgets,
        ConstraintInspectorWidget* parent)
{
}

void ScenarioConstraintInspectorDelegate::addWidgets_post(
        std::list<QWidget*>& widgets,
        ConstraintInspectorWidget* parent)
{
}

void ScenarioConstraintInspectorDelegate::on_defaultDurationChanged(
        OngoingCommandDispatcher& dispatcher,
        const TimeValue& val,
        ExpandMode expandmode) const
{
    auto& scenario = *safe_cast<Scenario::ScenarioModel*>(m_model.parent());
    dispatcher.submitCommand<MoveEventMeta>(
            scenario,
            scenario.state(m_model.endState()).eventId(),
            m_model.startDate() + val,
            expandmode);
}

