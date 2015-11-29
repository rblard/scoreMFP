#pragma once
#include <memory>

#include <iscore/statemachine/GraphicsSceneToolPalette.hpp>
#include <iscore/plugins/customfactory/FactoryInterface.hpp>
#include <iscore/tools/std/StdlibWrapper.hpp>

class ConstraintModel;
class ScenarioDocumentPresenter;
class DisplayedElementsToolPaletteFactory : public iscore::FactoryInterfaceBase
{
         ISCORE_FACTORY_DECL("DisplayedElementsToolPaletteFactory")
    public:
        virtual ~DisplayedElementsToolPaletteFactory();

         virtual bool matches(
                 const ConstraintModel& constraint) const = 0;

        virtual std::unique_ptr<GraphicsSceneToolPalette> make(
                 ScenarioDocumentPresenter& pres,
                 const ConstraintModel& constraint) = 0;
};
