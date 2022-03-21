#ifndef SERVERMODEL_HPP_
#define SERVERMODEL_HPP_

#include "umbridge/external/httplib.h"

#include "umbridge/Model.hpp"

namespace umbridge {

/// A server model
class ServerModel : Model {
public:

  /// Construct
  inline ServerModel() {}

  virtual ~ServerModel() = default;
private:

  httplib::Server svr;
};

} // namespace umbridge

#endif
