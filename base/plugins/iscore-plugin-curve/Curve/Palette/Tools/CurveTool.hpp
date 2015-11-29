#pragma once
#include <Curve/Segment/CurveSegmentModel.hpp>
#include <Curve/Segment/CurveSegmentView.hpp>
#include <Curve/Point/CurvePointModel.hpp>
#include <Curve/Point/CurvePointView.hpp>

#include <iscore/statemachine/GraphicsSceneTool.hpp>
#include <QGraphicsItem>
#include <chrono>
namespace Curve
{
class ToolPalette;
class CurveTool : public GraphicsSceneTool<Curve::Point>
{
    public:
        CurveTool(const Curve::ToolPalette&);

    protected:
        template<typename PointFun,
                 typename SegmentFun,
                 typename NothingFun>
        void mapTopItem(
                QPointF scenePoint,
                const QGraphicsItem* pressedItem,
                PointFun pt_fun,
                SegmentFun seg_fun,
                NothingFun nothing_fun) const
        {
            if(!pressedItem)
            {
                nothing_fun();
                return;
            }

            switch(pressedItem->type())
            {
                case CurvePointView::static_type():
                {
                    auto pt = safe_cast<const CurvePointView*>(pressedItem);
                    if(pt->contains(pt->mapFromScene(scenePoint)))
                        pt_fun(pt);
                    break;
                }

                case CurveSegmentView::static_type():
                {
                    auto segt = safe_cast<const CurveSegmentView*>(pressedItem);
                    if(segt->contains(segt->mapFromScene(scenePoint)))
                    {
                        seg_fun(segt);
                    }
                    break;
                }

                default:
                {
                    nothing_fun();
                    break;
                }
            }
        }

        const Curve::ToolPalette& m_parentSM;
};



}
