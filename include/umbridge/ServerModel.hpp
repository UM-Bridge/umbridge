#ifndef SERVERMODEL_HPP_
#define SERVERMODEL_HPP_

#include "umbridge/external/httplib.h"

#include "umbridge/Model.hpp"

namespace umbridge {

/// A server model
class ServerModel {
public:

  /// Construct
  ServerModel(std::string host, int port);

  virtual ~ServerModel() = default;
private:

  httplib::Server svr;
};

} // namespace umbridge

#endif
