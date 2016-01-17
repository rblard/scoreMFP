#pragma once

#include <QWidget>

#include <State/Expression.hpp>

namespace Inspector
{
class InspectorWidgetBase;
}
class QPushButton;

namespace Scenario
{
class ExpressionEditorWidget;
class TimeNodeModel;
namespace Command
{
class TriggerCommandFactoryList;
}
class TriggerInspectorWidget final : public QWidget
{
    public:
        TriggerInspectorWidget(
                const Command::TriggerCommandFactoryList& fact,
                const TimeNodeModel& object,
                Inspector::InspectorWidgetBase* parent);

        void on_triggerChanged();

        void createTrigger();
        void removeTrigger();

        void on_triggerActiveChanged();
        void HideRmButton();

        void updateExpression(const State::Trigger&);

    private:
        const Command::TriggerCommandFactoryList& m_triggerCommandFactory;
        const TimeNodeModel& m_model;

        Inspector::InspectorWidgetBase* m_parent;

        QPushButton* m_addTrigBtn{};
        QPushButton* m_rmTrigBtn{};

        ExpressionEditorWidget* m_exprEditor{};
};
}
