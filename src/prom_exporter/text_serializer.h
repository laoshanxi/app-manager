#pragma once

#include <iosfwd>
#include <string>
#include <vector>


#include "metric_family.h"
#include "serializer.h"

namespace prometheus {

class TextSerializer : public Serializer {
 public:
  using Serializer::Serialize;
  void Serialize(std::ostream& out,
                 const std::vector<MetricFamily>& metrics) const override;
};

}  // namespace prometheus
