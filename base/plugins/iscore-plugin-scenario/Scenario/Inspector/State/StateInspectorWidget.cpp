#include <Inspector/InspectorSectionWidget.hpp>
#include <Scenario/Commands/Event/SplitEvent.hpp>
#include <Scenario/DialogWidget/MessageTreeView.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/State/StateModel.hpp>
#include <Scenario/Process/ScenarioModel.hpp>

#include <boost/optional/optional.hpp>
#include <iscore/widgets/MarginLess.hpp>
#include <QtAlgorithms>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QVector>
#include <QWidget>
#include <algorithm>

#include <Inspector/InspectorWidgetBase.hpp>
#include "StateInspectorWidget.hpp"
#include <iscore/command/Dispatchers/CommandDispatcher.hpp>
#include <iscore/tools/ModelPath.hpp>
#include <iscore/tools/NotifyingMap.hpp>
#include <iscore/tools/SettableIdentifier.hpp>
#include <Scenario/Inspector/SelectionButton.hpp>
#include <iscore/document/DocumentContext.hpp>

#include <QSizePolicy>

namespace iscore {
class Document;
}  // namespace iscore

namespace Scenario
{
StateInspectorWidget::StateInspectorWidget(
        const StateModel& object,
        const iscore::DocumentContext& doc,
        QWidget *parent):
    QWidget {parent},
    m_model {object},
    m_commandDispatcher(new CommandDispatcher<>{doc.commandStack}),
    m_selectionDispatcher(new iscore::SelectionDispatcher{doc.selectionStack})
{
    setObjectName("StateInspectorWidget");
    setParent(parent);

    auto lay = new iscore::MarginLess<QVBoxLayout>{this};

    updateDisplayedValues();
}

void StateInspectorWidget::updateDisplayedValues()
{
    // Cleanup
    // OPTIMIZEME
    qDeleteAll(m_properties);
    m_properties.clear();

    auto widget = new QWidget;
    auto lay = new iscore::MarginLess<QFormLayout>{widget};
    // State id
    //lay->addRow("Id", new QLabel{QString::number(m_model.id().val().get())});

    auto scenar = dynamic_cast<ScenarioInterface*>(m_model.parent());
    ISCORE_ASSERT(scenar);

    auto event = m_model.eventId();

    // State setup
    m_stateSection = new Inspector::InspectorSectionWidget{"States", false, this};
    auto tv = new MessageTreeView{m_model,
                                m_stateSection};

    m_stateSection->addContent(tv);
    m_properties.push_back(m_stateSection);

    auto linkWidget = new QWidget;
    auto linkLay = new iscore::MarginLess<QHBoxLayout>{linkWidget};

    if(event)
    {
        auto btn = SelectionButton::make(
                    tr("Event"),
                &scenar->event(event),
                selectionDispatcher(),
                this);
        linkLay->addStretch(1);
        linkLay->addWidget(btn);
    }

    // Constraints setup
    if(m_model.previousConstraint())
    {
        auto btn = SelectionButton::make(
                    tr("Prev. Cstr"),
                &scenar->constraint(m_model.previousConstraint()),
                selectionDispatcher(),
                this);

        linkLay->addWidget(btn);
    }
    if(m_model.nextConstraint())
    {
        auto btn = SelectionButton::make(
                    tr("Next Cstr"),
                &scenar->constraint(m_model.nextConstraint()),
                selectionDispatcher(),
                this);

        linkLay->addWidget(btn);
    }
    linkLay->addStretch(1);

    m_stateSection->addContent(linkWidget);
    m_properties.push_back(widget);

    for(auto w : m_properties)
        this->layout()->addWidget(w);

//    updateAreaLayout(m_properties);
//    m_stateSection->expand();
}

void StateInspectorWidget::splitEvent()
{
    auto scenar = dynamic_cast<const Scenario::ScenarioModel*>(m_model.parent());
    if (scenar)
    {
        auto& parentEvent = scenar->events.at(m_model.eventId());
        if(parentEvent.states().size() > 1)
        {
            auto cmd = new Scenario::Command::SplitEvent{
                        *scenar,
                        m_model.eventId(),
                        {m_model.id()}};

            m_commandDispatcher->submitCommand(cmd);
        }
    }
}

}
