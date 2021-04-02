#pragma once
#include <Process/GenericProcessFactory.hpp>
#include <Process/WidgetLayer/WidgetProcessFactory.hpp>

#include <Pd/Inspector/PdInspectorWidget.hpp>
#include <Pd/PdProcess.hpp>
#include <Control/DefaultEffectItem.hpp>
#include <Effect/EffectFactory.hpp>
#include <QProcess>
namespace Pd
{
struct UiWrapper : public QWidget
{
  QPointer<const ProcessModel> m_model;
  QProcess m_process;
  UiWrapper(
      const ProcessModel& proc,
      const score::DocumentContext& ctx,
      QWidget* parent)
    : m_model{&proc}
  {
    setGeometry(0, 0, 0, 0);

    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &QWidget::deleteLater);
    connect(&proc, &IdentifiedObjectAbstract::identified_object_destroying,
            this, &QWidget::deleteLater);
    const auto& bin = locatePdBinary();
    if(!bin.isEmpty())
    {
      m_process.start(bin, {proc.script()});
    }
  }

  void closeEvent(QCloseEvent* event) override
  {
    QPointer<UiWrapper> p(this);
    if(m_model)
    {
      const_cast<QWidget*&>(m_model->externalUI) = nullptr;
      m_model->externalUIVisible(false);
    }
    if (p)
      QWidget::closeEvent(event);
  }

  ~UiWrapper()
  {
    if(m_model)
    {
      const_cast<QWidget*&>(m_model->externalUI) = nullptr;
      m_model->externalUIVisible(false);
    }
    m_process.terminate();
  }
};

using LayerFactory = Process::EffectLayerFactory_T<
    Pd::ProcessModel,
    Process::DefaultEffectItem,
    UiWrapper>;
}