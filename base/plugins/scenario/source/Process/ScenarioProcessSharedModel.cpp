#include "ScenarioProcessSharedModel.hpp"

#include "Process/Temporal/TemporalScenarioProcessViewModel.hpp"

#include "Document/Event/EventModel.hpp"
#include "Document/Event/EventData.hpp"
#include "Document/Constraint/ConstraintModel.hpp"
#include "Document/TimeNode/TimeNodeModel.hpp"

#include <API/Headers/Editor/Scenario.h>

#include <QDebug>

ScenarioProcessSharedModel::ScenarioProcessSharedModel(id_type<ProcessSharedModelInterface> id, QObject* parent):
	ProcessSharedModelInterface{id, "ScenarioProcessSharedModel", parent},
	//m_scenario{nullptr},
	m_startEventId{0} // Always
{
	auto event = new EventModel{m_startEventId, this};
	auto timeNode = new TimeNodeModel{id_type<TimeNodeModel>(0),
					event->date(),
					this};
	timeNode->addEvent(m_startEventId);
	timeNode->setY(event->heightPercentage());

	// TODO jm : TimeNode::addEvent devrait faire ça
	event->changeTimeNode(id_type<TimeNodeModel>(0));

	addEvent(event);
	m_timeNodes.push_back(timeNode); // TODO : addTimeNode qui envoie un signal.


	//TODO demander à Clément si l'élément de fin sert vraiment à qqch ?
	//m_events.push_back(new EventModel(1, this));
}

ScenarioProcessSharedModel::~ScenarioProcessSharedModel()
{
	//if(m_scenario) delete m_scenario;
}

ProcessViewModelInterface* ScenarioProcessSharedModel::makeViewModel(id_type<ProcessViewModelInterface> viewModelId,
																	 QObject* parent)
{
	auto scen = new TemporalScenarioProcessViewModel(viewModelId, this, parent);
	makeViewModel_impl(scen);
	return scen;
}


void ScenarioProcessSharedModel::makeViewModel_impl(ScenarioProcessSharedModel::view_model_type* scen)
{
	addViewModel(scen);

	connect(scen, &TemporalScenarioProcessViewModel::destroyed,
			this, &ScenarioProcessSharedModel::on_viewModelDestroyed);

	connect(this, &ScenarioProcessSharedModel::constraintRemoved,
			scen, &view_model_type::on_constraintRemoved);

	connect(this, &ScenarioProcessSharedModel::eventCreated,
			scen, &view_model_type::eventCreated);
	connect(this, &ScenarioProcessSharedModel::timeNodeCreated,
			scen, &view_model_type::timeNodeCreated);
	connect(this, &ScenarioProcessSharedModel::eventRemoved,
			scen, &view_model_type::eventDeleted);
    connect(this, &ScenarioProcessSharedModel::timeNodeRemoved,
            scen, &view_model_type::timeNodeDeleted);
    connect(this, &ScenarioProcessSharedModel::eventMoved,
			scen, &view_model_type::eventMoved);
	connect(this, &ScenarioProcessSharedModel::constraintMoved,
			scen, &view_model_type::constraintMoved);
}


//////// Creation ////////
void ScenarioProcessSharedModel::createConstraintBetweenEvents(id_type<EventModel> startEventId,
															   id_type<EventModel> endEventId,
															   id_type<ConstraintModel> newConstraintModelId,
															   id_type<AbstractConstraintViewModel> newConstraintFullViewId)
{
	auto sev = this->event(startEventId);
	auto eev = this->event(endEventId);
	auto inter = new ConstraintModel{newConstraintModelId,
									 newConstraintFullViewId,
									 this};

/*	auto ossia_tn0 = sev->apiObject();
	auto ossia_tn1 = eev->apiObject();
	auto ossia_tb = inter->apiObject();

	m_scenario->addTimeBox(*ossia_tb,
						   *ossia_tn0,
						   *ossia_tn1);
*/
	// Error checking if it did not go well ? Rollback ?
	// Else...
	inter->setStartEvent(sev->id());
	inter->setEndEvent(eev->id());
	inter->setStartDate(sev->date());
	inter->setDefaultDuration(eev->date() - sev->date());

	sev->addNextConstraint(newConstraintModelId);
	eev->addPreviousConstraint(newConstraintModelId);

	// From now on everything must be in a valid state.
	addConstraint(inter);
	emit constraintCreated(inter->id());
}

void
ScenarioProcessSharedModel::createConstraintAndEndEventFromEvent(id_type<EventModel> startEventId,
																 int constraint_duration,
																 double heightPos,
																 id_type<ConstraintModel> newConstraintId,
																 id_type<AbstractConstraintViewModel> newConstraintFullViewId,
																 id_type<EventModel> newEventId,
																 id_type<TimeNodeModel> newTimeNodeId)
{
	auto startEvent = this->event(startEventId);

	auto constraint = new ConstraintModel{newConstraintId,
					  newConstraintFullViewId,
					  this->event(startEventId)->heightPercentage(),
					  this};
	auto event = new EventModel{newEventId,
				 heightPos,
				 this};


	if (startEventId == m_startEventId)
	{
		constraint->setHeightPercentage(heightPos);
	}
	else
	{
		constraint->setHeightPercentage((heightPos + startEvent->heightPercentage()) / 2);
	}

	// TEMPORARY :
	constraint->setStartDate(this->event(startEventId)->date());
	constraint->setDefaultDuration(constraint_duration);
	event->setDate(constraint->startDate() + constraint->defaultDuration());

//	auto ossia_tn0 = this->event(startEventId)->apiObject();
//	auto ossia_tn1 = event->apiObject();
//	auto ossia_tb = constraint->apiObject();

//	m_scenario->addTimeBox(*ossia_tb,
//						   *ossia_tn0,
//						   *ossia_tn1);

	// Error checking if it did not go well ? Rollback ?
	// Else...
	constraint->setStartEvent(startEventId);
	constraint->setEndEvent(event->id());

	// TIMENODE
	auto timeNode = new TimeNodeModel{newTimeNodeId,
					event->date(),
					this};
	timeNode->addEvent(newEventId);
	timeNode->setY(event->heightPercentage());

	// TODO jm : TimeNode::addEvent devrait faire ça
	event->changeTimeNode(newTimeNodeId);

	// From now on everything must be in a valid state.
	m_events.push_back(event);
	m_constraints.push_back(constraint);
	m_timeNodes.push_back(timeNode);

	emit eventCreated(event->id());
	emit constraintCreated(constraint->id());
	emit timeNodeCreated(timeNode->id());

	// link constraint with event
	event->addPreviousConstraint(newConstraintId);
	this->event(startEventId)->addNextConstraint(newConstraintId);
	event->setVerticalExtremity(constraint->id(),
								constraint->heightPercentage());
	this->event(startEventId)->setVerticalExtremity(constraint->id(),
													constraint->heightPercentage());
}

void ScenarioProcessSharedModel::moveEventAndConstraint(id_type<EventModel> eventId,
														int absolute_time,
														double heightPosition)
{
	// resize previous constraint
	if (! event(eventId)->previousConstraints().isEmpty() )
	{
		auto ev = event(eventId);

		int time = absolute_time - ev->date();

		ev->setHeightPercentage(heightPosition);
		ev->translate(time);

		auto tn = timeNode(ev->timeNode());
		tn->setDate(ev->date());
		tn->setY(heightPosition);

		for (auto& prevConstraintId : event(eventId)->previousConstraints())
		{
			auto prevConstraint = constraint(prevConstraintId);
			prevConstraint->setDefaultDuration(prevConstraint->defaultDuration() + time);
			emit constraintMoved(prevConstraintId);
		}

		emit eventMoved(eventId);

		QVector<id_type<EventModel>> already_moved_events;
		moveNextElements(eventId, time, already_moved_events);
	}
}

void ScenarioProcessSharedModel::moveConstraint(id_type<ConstraintModel> constraintId,
												int absolute_time,
												double heightPosition)
{
    constraint(constraintId)->setHeightPercentage(heightPosition);
    emit constraintMoved(constraintId);

	//auto eev = event(constraint(constraintId)->endEvent());
	auto sev = event(constraint(constraintId)->startEvent());

	// TODO start timenode, end timenode
	//auto etn = timeNode(eev->timeNode());
	//auto stn = timeNode(sev->timeNode());

	moveEventAndConstraint(sev->id(),
						   absolute_time,
						   sev->heightPercentage());
}

void ScenarioProcessSharedModel::moveNextElements(id_type<EventModel> firstEventMovedId,
												  int deltaTime,
												  QVector<id_type<EventModel>> &movedEvent)
{
	auto cur_event = event(firstEventMovedId);

	if (! cur_event->previousConstraints().isEmpty())
	{
		for (auto cons : cur_event->nextConstraints())
		{
			auto evId = constraint(cons)->endEvent();
			// if event not already moved
			if (movedEvent.indexOf(evId) == -1)
			{
				event(evId)->translate(deltaTime);
				movedEvent.push_back(evId);
				constraint(cons)->translate(deltaTime);

				// move timeNode
				auto tn = timeNode(event(evId)->timeNode());
				tn->setDate(event(evId)->date());

				emit eventMoved(evId);
				emit constraintMoved(cons);

				// adjust previous constraint width
				for (auto& prevConstraintId : event(evId)->previousConstraints())
				{
					auto prevConstraint = constraint(prevConstraintId);
					prevConstraint->setDefaultDuration(event(evId)->date() - prevConstraint->startDate());
					emit constraintMoved(prevConstraintId);
				}

				moveNextElements(evId, deltaTime, movedEvent);
			}
		}
	}
}

///////// DELETION //////////
#include <tools/utilsCPP11.hpp>
void ScenarioProcessSharedModel::removeConstraint(id_type<ConstraintModel> constraintId)
{
	auto cstr = constraint(constraintId);
	vec_erase_remove_if(m_constraints,
						[&constraintId] (ConstraintModel* model)
						{ return model->id() == constraintId; });

	emit constraintRemoved(constraintId);
	delete cstr;
}

void ScenarioProcessSharedModel::removeEvent(id_type<EventModel> eventId)
{
	// @todo : delete event in timeNode list (and timeNode if empty)
	auto ev = event(eventId);
	vec_erase_remove_if(m_events,
						[&eventId] (EventModel* model)
						{ return model->id() == eventId; });

    removeEventFromTimeNode(eventId);
	emit eventRemoved(eventId);
	delete ev;
}

void ScenarioProcessSharedModel::removeEventFromTimeNode(id_type<EventModel> eventId)
{
	for (auto& timeNode : m_timeNodes)
	{
		if ( timeNode->removeEvent(eventId) )
		{
            if (timeNode->isEmpty()) removeTimeNode(timeNode->id());
			return;
		}
    }
}

void ScenarioProcessSharedModel::removeTimeNode(id_type<TimeNodeModel> timeNodeId)
{
    auto tn = timeNode(timeNodeId);
    vec_erase_remove_if(m_timeNodes,
                        [&timeNodeId] (TimeNodeModel* model)
                        { return model->id() == timeNodeId; });

    emit timeNodeRemoved(timeNodeId);
    delete tn;
}

void ScenarioProcessSharedModel::undo_createConstraintAndEndEventFromEvent(id_type<ConstraintModel> constraintId)
{
	// @todo : delete event in timeNode list (and timeNode if empty)
	// End event suppression
	{
		auto end_event_id = this->constraint(constraintId)->endEvent();

        removeEventFromTimeNode(end_event_id);

		emit eventRemoved(end_event_id);
		// TODO careful with this.
		removeById(m_events, end_event_id);

		auto start_event = event(constraint(constraintId)->startEvent());
		start_event->removeNextConstraint(constraintId);
	}

	// Constraint suppression
	removeConstraint(constraintId);
}

void ScenarioProcessSharedModel::undo_createConstraintBetweenEvent(id_type<ConstraintModel> constraintId)
{
	auto end_event = event(this->constraint(constraintId)->endEvent());
	end_event->removePreviousConstraint(constraintId);

	auto start_event = event(this->constraint(constraintId)->startEvent());
	start_event->removeNextConstraint(constraintId);

	removeConstraint(constraintId);
}


/////////////////////////////
ConstraintModel* ScenarioProcessSharedModel::constraint(id_type<ConstraintModel> constraintId) const
{
	return findById(m_constraints, constraintId);
}

EventModel* ScenarioProcessSharedModel::event(id_type<EventModel> eventId) const
{
	return findById(m_events, eventId);
}

TimeNodeModel *ScenarioProcessSharedModel::timeNode(id_type<TimeNodeModel> timeNodeId) const
{
	return findById(m_timeNodes, timeNodeId);
}

EventModel* ScenarioProcessSharedModel::startEvent() const
{
	return event(m_startEventId);
}

EventModel* ScenarioProcessSharedModel::endEvent() const
{
	return event(m_endEventId);
}

std::vector<EventModel*> ScenarioProcessSharedModel::events() const
{
	return m_events;
}

void ScenarioProcessSharedModel::on_viewModelDestroyed(QObject* obj)
{
	removeViewModel(static_cast<ProcessViewModelInterface*>(obj));
}



void ScenarioProcessSharedModel::addConstraint(ConstraintModel* constraint)
{
	m_constraints.push_back(constraint);
	emit constraintCreated(constraint->id());
}

void ScenarioProcessSharedModel::addEvent(EventModel* event)
{
	m_events.push_back(event);
	emit eventCreated(event->id());
}
