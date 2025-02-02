// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "ColorReference.hpp"

#include <score/serialization/DataStreamVisitor.hpp>
#include <score/tools/Debug.hpp>

SCORE_SERALIZE_DATASTREAM_DEFINE(score::ColorRef);
namespace score
{
std::optional<ColorRef> ColorRef::ColorFromString(const QString& txt) noexcept
{
  auto res = score::Skin::instance().fromString(txt);

  if (res)
    return ColorRef(res);
  else
    return {};
}

std::optional<ColorRef> ColorRef::SimilarColor(QColor other) noexcept
{
  SCORE_TODO_("Load similar colors");
  return {};
}
}
