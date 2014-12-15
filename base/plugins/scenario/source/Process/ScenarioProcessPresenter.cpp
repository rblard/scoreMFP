#include "ScenarioProcessPresenter.hpp"

#include "Process/ScenarioProcessSharedModel.hpp"
#include "Process/ScenarioProcessViewModel.hpp"
#include "Process/ScenarioProcessView.hpp"
#include "Document/Constraint/ConstraintView.hpp"
#include "Document/Constraint/ConstraintData.hpp"
#include "Document/Constraint/ConstraintPresenter.hpp"
#include "Document/Constraint/ConstraintModel.hpp"
#include "Document/Constraint/Box/BoxView.hpp"
#include "Document/Constraint/Box/BoxPresenter.hpp"
#include "Document/Event/EventModel.hpp"
#include "Document/Event/EventPresenter.hpp"
#include "Document/Event/EventView.hpp"
#include "Document/Event/EventData.hpp"
#include "Commands/Scenario/CreateEventCommand.hpp"
#include "Commands/Scenario/CreateEventAfterEventCommand.hpp"
#include "Commands/Scenario/MoveEventCommand.hpp"
#include "Commands/Scenario/MoveConstraintCommand.hpp"

#include <tools/utilsCPP11.hpp>

#include <QDebug>
#include <QRectF>


ScenarioProcessPresenter::ScenarioProcessPresenter(ProcessViewModelInterface* model,
												   ProcessViewInterface* view,
												   QObject* parent):
	ProcessPresenterInterface{"ScenarioProcessPresenter", parent},
	m_viewModel{static_cast<ScenarioProcessViewModel*>(model)},
	m_view{static_cast<ScenarioProcessView*>(view)}
{
	/////// Setup of existing data
	// For each constraint & event, display' em
	for(auto constraint_model : m_viewModel->model()->constraints())
	{
		on_constraintCreated_impl(constraint_model);
	}

	for(auto event_model : m_viewModel->model()->events())
	{
		on_eventCreated_impl(event_model);
	}

	/////// Connections
	connect(this,	SIGNAL(elementSelected(QObject*)),
			parent, SIGNAL(elementSelected(QObject*)));

	connect(m_view, &ScenarioProcessView::scenarioPressed,
			this,	&ScenarioProcessPresenter::on_scenarioPressed);
	connect(m_view, &ScenarioProcessView::scenarioReleased,
			this,	&ScenarioProcessPresenter::on_scenarioReleased);

	connect(m_viewModel, &ScenarioProcessViewModel::eventCreated,
			this,		 &ScenarioProcessPresenter::on_eventCreated);
	connect(m_viewModel, &ScenarioProcessViewModel::eventDeleted,
			this,		 &ScenarioProcessPresenter::on_eventDeleted);
	connect(m_viewModel, &ScenarioProcessViewModel::constraintCreated,
			this,		 &ScenarioProcessPresenter::on_constraintCreated);
	connect(m_viewModel, &ScenarioProcessViewModel::constraintDeleted,
			this,		 &ScenarioProcessPresenter::on_constraintDeleted);
	connect(m_viewModel, &ScenarioProcessViewModel::eventMoved,
			this,		 &ScenarioProcessPresenter::on_eventMoved);
	connect(m_viewModel, &ScenarioProcessViewModel::constraintMoved,
			this,		 &ScenarioProcessPresenter::on_constraintMoved);
}

int ScenarioProcessPresenter::id() const
{
	return m_viewModel->model()->id();
}

int ScenarioProcessPresenter::currentlySelectedEvent() const
{
	return m_currentlySelectedEvent;
}

void ScenarioProcessPresenter::on_eventCreated(int eventId)
{
	on_eventCreated_impl(m_viewModel->model()->event(eventId));
}

void ScenarioProcessPresenter::on_constraintCreated(int constraintId)
{
	on_constraintCreated_impl(m_viewModel->model()->constraint(constraintId));
}

void ScenarioProcessPresenter::on_eventDeleted(int eventId)
{
	removeFromVectorWithId(m_events, eventId);
	m_view->update();
}

void ScenarioProcessPresenter::on_constraintDeleted(int constraintId)
{
	removeFromVectorWithId(m_constraints, constraintId);
	m_view->update();
}

void ScenarioProcessPresenter::on_eventMoved(int eventId)
{
	auto rect = m_view->boundingRect();

	for(auto ev : m_events) {
		if(ev->id() == eventId ) {
			ev->view()->setTopLeft({rect.x() + ev->model()->date(),
									rect.y() + rect.height() * ev->model()->heightPercentage()});
			ev->model()->updateVerticalLink();
		}
	}
	m_view->update();
}

void ScenarioProcessPresenter::on_constraintMoved(int constraintId)
{

	auto rect = m_view->boundingRect();

	for(auto inter : m_constraints) {
		if(inter->id() == constraintId ) {
			inter->view()->setTopLeft({rect.x() + inter->model()->startDate(),
									rect.y() + rect.height() * inter->model()->heightPercentage()});
			inter->view()->m_rect.setWidth(inter->model()->width());
		}
	}
	m_view->update();
}


void ScenarioProcessPresenter::on_scenarioPressed(QPointF point)
{   // @todo maybe better to create event on mouserelease ? And only show a "fake" event + interval on mousepress.
	auto cmd = new CreateEventCommand(ObjectPath::pathFromObject("BaseConstraintModel",
																m_viewModel->model()),
									 point.x(),
									 (point - m_view->boundingRect().topLeft() ).y() / m_view->boundingRect().height() );
	this->submitCommand(cmd);
}

void ScenarioProcessPresenter::on_scenarioReleased(QPointF point)
{
	EventData data{};
	data.eventClickedId = m_events.back()->id();
	data.x = point.x() - m_events.back()->model()->date();
	data.y = point.y();
	if (point.x() - m_events.back()->model()->date() > 20 ) // @todo use a const to do that !
		createConstraintAndEventFromEvent(data);
}

void ScenarioProcessPresenter::on_askUpdate()
{
	m_view->update();
}

void ScenarioProcessPresenter::setCurrentlySelectedEvent(int arg)
{
	if (m_currentlySelectedEvent != arg) {
		m_currentlySelectedEvent = arg;
		emit currentlySelectedEventChanged(arg);
	}
}

void ScenarioProcessPresenter::createConstraintAndEventFromEvent(EventData data)
{
	data.relativeY = (data.y - m_view->boundingRect().topLeft().y())/m_view->boundingRect().height();
	auto cmd = new CreateEventAfterEventCommand(ObjectPath::pathFromObject("BaseConstraintModel",
																		   m_viewModel->model()),
												data);

	submitCommand(cmd);
}

void ScenarioProcessPresenter::moveEventAndConstraint(EventData data)
{
	data.relativeY = (data.y - m_view->boundingRect().topLeft().y())/m_view->boundingRect().height();
	auto cmd = new MoveEventCommand(ObjectPath::pathFromObject("BaseConstraintModel",
															   m_viewModel->model()),
									data);
	submitCommand(cmd);
}

void ScenarioProcessPresenter::moveConstraint(ConstraintData data)
{
	data.relativeY = data.y / m_view->boundingRect().height();
	auto cmd = new MoveConstraintCommand(ObjectPath::pathFromObject("BaseConstraintModel",
															   m_viewModel->model()),
									data);

	submitCommand(cmd);
}




void ScenarioProcessPresenter::on_eventCreated_impl(EventModel* event_model)
{
	auto rect = m_view->boundingRect();

	auto event_view = new EventView{m_view};
	auto event_presenter = new EventPresenter{event_model,
											  event_view,
											  this};
	event_view->setTopLeft({rect.x() + event_model->date(),
							rect.y() + rect.height() * event_model->heightPercentage()});

	m_events.push_back(event_presenter);

	connect(event_presenter, &EventPresenter::eventSelected,
			this,			 &ScenarioProcessPresenter::setCurrentlySelectedEvent);
	connect(event_presenter, &EventPresenter::eventReleasedWithControl,
			this,			 &ScenarioProcessPresenter::createConstraintAndEventFromEvent);
	connect(event_presenter, &EventPresenter::eventReleased,
			this,			 &ScenarioProcessPresenter::moveEventAndConstraint);
	connect(event_presenter, &EventPresenter::elementSelected,
			this,			 &ScenarioProcessPresenter::elementSelected);

	connect(event_presenter, &EventPresenter::linesExtremityChange,
			[event_view, this] (double top, double bottom) {
				event_view->setLinesExtremity(top * m_view->boundingRect().height(), bottom * m_view->boundingRect().height());
			});
}

void ScenarioProcessPresenter::on_constraintCreated_impl(ConstraintModel* constraint_model)
{
	auto rect = m_view->boundingRect();

	auto constraint_view = new ConstraintView{m_view};
	auto constraint_presenter = new ConstraintPresenter{constraint_model,
													constraint_view,
													this};

	constraint_view->setTopLeft({rect.x() + constraint_model->startDate(),
							   rect.y() + rect.height() * constraint_model->heightPercentage()});

	m_constraints.push_back(constraint_presenter);

	connect(constraint_presenter,	&ConstraintPresenter::constraintReleased,
			this,					&ScenarioProcessPresenter::moveConstraint);
	connect(constraint_presenter,	&ConstraintPresenter::submitCommand,
			this,					&ScenarioProcessPresenter::submitCommand);
	connect(constraint_presenter,	&ConstraintPresenter::elementSelected,
			this,					&ScenarioProcessPresenter::elementSelected);


	connect(constraint_presenter,	&ConstraintPresenter::askUpdate,
			this,					&ScenarioProcessPresenter::on_askUpdate);
}
